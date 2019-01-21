/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/film.h>
#include <lm/logger.h>
#include <lm/serial.h>

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
};

class Film_Bitmap final : public Film {
private:
    int w_;
    int h_;
    std::vector<AtomicWrapper<Vec3>> data_;
    std::vector<Vec3> dataTemp_;  // Temporary buffer for external reference

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(w_, h_, data_);
    }

public:
    virtual bool construct(const Json& prop) override {
        w_ = prop["w"];
        h_ = prop["h"];
        data_.assign(w_*h_, {});
        return true;
    }

    virtual FilmSize size() const override {
        return { w_, h_ };
    }

    virtual void setPixel(int x, int y, Vec3 v) override {
        data_[y*w_ + x].update(v);
    }

    virtual bool save(const std::string& outpath) const override {
        LM_INFO("Saving image [file='{}']", outpath);
        LM_INDENT();

        // Create directory if not found
        const auto parent = std::filesystem::path(outpath).parent_path();
        if (!std::filesystem::exists(parent)) {
            LM_INFO("Creating directory [path='{}']", parent.string());
            if (!std::filesystem::create_directory(parent)) {
                LM_INFO("Failed to create directory [path='{}']", parent.string());
                return false;
            }
        }

        // Save file
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
        fprintf(f, "PF\n%d %d\n-1\n", w_, h_);
        std::vector<float> d(w_*h_*3);
        for (int y = 0; y < h_; y++) {
            for (int x = 0; x < w_; x++) {
                for (int i = 0; i < 3; i++) {
                    d[3*(y*w_+x)+i] = float(data_[y*w_+x].v_.load()[i]);
                }
            }
        }
        fwrite(d.data(), 4, d.size(), f);
        fclose(f);
        return true;
    }

    virtual FilmBuffer buffer() override {
        dataTemp_.clear();
        for (const auto& v : data_) {
            dataTemp_.push_back(v.v_);
        }
        return FilmBuffer{ w_, h_, &dataTemp_[0].x };
    }
};

LM_COMP_REG_IMPL(Film_Bitmap, "film::bitmap");

LM_NAMESPACE_END(LM_NAMESPACE)
