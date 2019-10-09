/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/film.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <lm/parallel.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// Wrapper for std::atomic.
// Extending std::vector with std::atomic generates a compilation error
// because std::atomic is neither copy nor move constructable.
// cf. https://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c
template <typename T>
struct AtomicWrapper {
    std::atomic<T> v_;

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(v_);
    }

    AtomicWrapper() = default;
    AtomicWrapper(T& v)
        : v_(v) {}
    AtomicWrapper(const std::atomic<T>& v)
        : v_(v.load()) {}
    AtomicWrapper(const AtomicWrapper& o)
        : v_(o.v_.load()) {}

    AtomicWrapper& operator=(const AtomicWrapper& o) {
        v_.store(o.v_.load());
        return *this;
    }
    
    void update(const T& src) {
        auto expected = v_.load();
        while (!v_.compare_exchange_weak(expected, src));
    }

    void add(const T& v) {
        auto expected = v_.load();
        while (!v_.compare_exchange_weak(expected, expected + v));
    }

    void updateWithFunc(const std::function<T(T curr)>& updateFunc) {
        auto expected = v_.load();
        while (!v_.compare_exchange_weak(expected, updateFunc(expected)));
    }
};

// ------------------------------------------------------------------------------------------------

// Helper functions on image manipulaion
namespace image {
    // Write image as .pfm file
    bool writePfm(const std::string& outpath, int w, int h, const std::vector<float>& d) {
        FILE *f;
        int err;
        #if LM_COMPILER_MSVC
        if ((err = fopen_s(&f, outpath.c_str(), "wb")) != 0) {
            LM_ERROR("Failed to open [file='{}',errorno='{}']", outpath, err);
            return false;
        }
        #else
        if ((f = fopen(outpath.c_str(), "wb")) == nullptr) {
            LM_ERROR("Failed to open [file='{}']", outpath);
            return false;
        }
        #endif
        fprintf(f, "PF\n%d %d\n-1\n", w, h);
        fwrite(d.data(), 4, d.size(), f);
        fclose(f);
        return true;
    }

    // Sanity check of an image. If the image contains invalid values like INF or NAN,
    // this function returns false.
    bool sanityCheck(int w, int h, const std::vector<float>& d) {
        constexpr int MaxInvalidPixels = 10;
        int invalidPixels = 0;
        for (int i = 0; i < w * h; i++) {
            const int x = i % w;
            const int y = i / w;
            const auto v = d[i];
            if (std::isnan(v)) {
                LM_WARN("Found an invalid pixel [type='NaN', x={}, y={}]", x, y);
                invalidPixels++;
            }
            else if (std::isinf(v)) {
                LM_WARN("Found an invalid pixel [type='Inf', x={}, y={}]", x, y);
                invalidPixels++;
            }
            if (invalidPixels >= MaxInvalidPixels) {
                LM_WARN("Outputs more than >{} entries are omitted.", MaxInvalidPixels);
                break;
            }
        }
        return invalidPixels > 0;
    }
}

// ------------------------------------------------------------------------------------------------

/*
\rst
.. function:: film::bitmap

   Bitmap film.

   :param int w: Width of the film.
   :param int h: Height of the film.

   This component implements thread-safe bitmap film.
   The invocation of :cpp:func:`lm::Film::setPixel()` function is thread safe.
\endrst
*/
class Film_Bitmap final : public Film {
private:
    int w_;
    int h_;
    int quality_;
    std::vector<AtomicWrapper<Vec3>> data_;
    std::vector<Vec3> dataTemp_;  // Temporary buffer for external reference

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(w_, h_, quality_, data_);
    }

public:
    virtual bool construct(const Json& prop) override {
        w_ = json::value<int>(prop, "w");
        h_ = json::value<int>(prop, "h");
        quality_ = json::value<int>(prop, "quality", 90);
        data_.assign(w_*h_, {});
        return true;
    }

    virtual FilmSize size() const override {
        return { w_, h_ };
    }

    virtual long long numPixels() const override {
        return w_ * h_;
    }

    virtual void setPixel(int x, int y, Vec3 v) override {
        data_[y*w_ + x].update(v);
    }

    virtual bool save(const std::string& outpath) const override {
        // Disable floating-point exception for stb_image
        exception::ScopedDisableFPEx disableFp_;

        LM_INFO("Saving image [file='{}']", outpath);
        LM_INDENT();

        // Create directory if not found
        const auto parent = fs::path(outpath).parent_path();
        if (!parent.empty() && !fs::exists(parent)) {
            LM_INFO("Creating directory [path='{}']", parent.string());
            if (!fs::create_directories(parent)) {
                LM_INFO("Failed to create directory [path='{}']", parent.string());
                return false;
            }
        }

        // Save file
        // Check extension of the output file
        const auto ext = fs::path(outpath).extension().string();
        if (ext == ".png") {
            const auto data = copy<unsigned char>(true);
            if (!stbi_write_png(outpath.c_str(), w_, h_, 3, data.data(), w_*3)) {
                return false;
            }
        }
        #if 0
        else if (ext == ".jpg") {
            const auto data = copy<unsigned char>(true);
            if (!stbi_write_jpg(outpath.c_str(), w_, h_, 3, data.data(), quality_)) {
                return false;
            }
        }
        #endif
        else if (ext == ".hdr") {
            auto data = copy<float>(true);
            image::sanityCheck(w_, h_, data);
            if (!stbi_write_hdr(outpath.c_str(), w_, h_, 3, data.data())) {
                return false;
            }
        }
        else if (ext == ".pfm") {
            const auto data = copy<float>(false);
            image::sanityCheck(w_, h_, data);
            if (!image::writePfm(outpath, w_, h_, data)) {
                return false;
            }
        }
        else {
            LM_ERROR("Invalid extension [ext='{}']", ext);
            return false;
        }

        return true;
    }

    virtual FilmBuffer buffer() override {
        dataTemp_.clear();
        for (const auto& v : data_) {
            dataTemp_.push_back(v.v_);
        }
        return FilmBuffer{ w_, h_, &dataTemp_[0].x };
    }

    virtual void accum(const Film* film_) override {
        const auto* film = dynamic_cast<const Film_Bitmap*>(film_);
        if (!film) {
            LM_ERROR("Could not accumuate film. Invalid film type.");
            return;
        }
        if (w_ != film->w_ || h_ != film->h_) {
            LM_ERROR("Film size is different [expected='({},{})', actual='({},{})']", w_, h_, film->w_, film->h_);
            return;
        }
        for (int i = 0; i < w_*h_; i++) {
            const auto v = film->data_[i].v_.load();
            data_[i].add(v);
        }
    }

    virtual void splatPixel(int x, int y, Vec3 v) override {
        data_[y*w_+x].add(v);
    }

    virtual void updatePixel(int x, int y, const PixelUpdateFunc& updateFunc) override {
        data_[y*w_+x].updateWithFunc(updateFunc);
    }

    virtual void rescale(Float s) override {
        parallel::foreach(w_ * h_, [&](long long i, int) {
            data_[i].v_ = data_[i].v_.load() * s;
        });
    }

    virtual void clear() override {
        data_.assign(w_*h_, {});
    }

private:
    template <typename T>
    std::vector<T> copy(bool flip) const {
        std::vector<T> v(w_*h_*3, {});
        for (int y = 0; y < h_; y++) {
            const int yy = !flip ? y : h_-y-1;
            for (int x = 0; x < w_; x++) {
                for (int i = 0; i < 3; i++) {
                    const Float t = data_[y*w_+x].v_.load()[i];
                    if constexpr (std::is_same_v<T, float>) {
                        v[3*(yy*w_+x)+i] = T(t);
                    }
                    if constexpr (std::is_same_v<T, unsigned char>) {
                        const auto t2 = std::pow(t, 1_f/2.2_f);
                        v[3*(yy*w_+x)+i] = (unsigned char)glm::clamp(int(256_f*t2), 0, 255);
                    }
                }
            }
        }
        return std::move(v);
    }
};

LM_COMP_REG_IMPL(Film_Bitmap, "film::bitmap");

LM_NAMESPACE_END(LM_NAMESPACE)
