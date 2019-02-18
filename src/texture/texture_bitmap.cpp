/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/texture.h>
#include <lm/logger.h>
#include <lm/serial.h>
#pragma warning(push)
#pragma warning(disable:4244) // possible loss of data
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

#if 0
#if LM_COMPILER_MSVC
int bswap(int x) { return _byteswap_ulong(x); }
#elif LM_COMPILER_GCC
int bswap(int x) { return __builtin_bswap32(x); }
#endif

#if LM_PLATFORM_WINDOWS
std::string sanitizeDirectorySeparator(std::string p) {
    return p;
}
#else
std::string sanitizeDirectorySeparator(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}
#endif

struct Bitmap_PXM {
    int w;                  // Width of the texture
    int h;                  // Height of the texture
    std::vector<Float> cs;  // Colors
    std::vector<Float> as;  // Alphas

public:
    template <typename Archive>
    void serialize(Archive& ar) {
        ar(w, h, cs, as);
    }
    
private:
    // Calculate pixel coordinate of the vertically-flipped image
    int flip(int i) {
        const int j = i / 3;
        const int x = j % w;
        const int y = j / w;
        return 3*((h-y-1)*w + x) + i%3;
    }

    // Post procses a pixel for ppm textures
    Float postprocess(int i, Float e, std::vector<uint8_t>& ct) {
        // Gamma expansion
        return std::pow(Float(ct[flip(i)]) / e, 2.2_f);
    }

    // Post procses a pixel for pmf textures
    Float postprocess(int i, Float e, std::vector<float>& ct) {
        if (e < 0) {
            return ct[flip(i)];
        }
        auto m = bswap(*(int32_t *)&ct[flip(i)]);
        return *(float *)&m;
    }

    // Load a ppm or a pfm texture
    template <typename T>
    bool loadpxm(std::vector<Float>& c, const std::string& p_, bool errorOnFailure = true) {
        const auto p = sanitizeDirectorySeparator(p_);
        LM_INFO("Loading texture [path='{}']", p);
        LM_INDENT();

        // Open image file
        FILE *f;
        {
            #if LM_COMPILER_MSVC
            fopen_s(&f, p.c_str(), "rb");
            #else
            f = fopen(p.c_str(), "rb");
            #endif
            if (!f) {
                if (errorOnFailure) {
                    LM_ERROR("Failed to load texture [path='{}']", p);
                    return false;
                }
                return true;
            }
        }

        // Read header
        double e;
        {
            #if LM_COMPILER_MSVC
            const auto ret = fscanf_s(f, "%*s %d %d %lf%*c", &w, &h, &e);
            #else
            const auto ret = fscanf(f, "%*s %d %d %lf%*c", &w, &h, &e);
            #endif
            if (ret != 3) {
                LM_ERROR("Invalid PXM header [path='{}']", p);
                return false;
            }
        }

        // Read data
        const int sz = w * h * 3;
        static std::vector<T> ctemp;
        {
            ctemp.assign(sz, 0);
            c.assign(sz, 0);
            #if LM_COMPILER_MSVC
            const auto ret = fread_s(ctemp.data(), sz*sizeof(T), sizeof(T), sz, f);
            #else
            const auto ret = fread(ctemp.data(), sizeof(T), sz, f);
            #endif
            if (ret != sz) {
                LM_ERROR("Invalid data [path='{}']", p);
                return false;
            }
        }
        
        // Postprocess
        for (int i = 0; i < sz; i++) {
            c[i] = postprocess(i, Float(e), ctemp);
        }

        fclose(f);
        return true;
    }

public:
    // Load pfm texture
    bool loadpfm(const std::string& p) {
        return loadpxm<float>(cs, p);
    }

    // Load ppm texture
    bool loadppm(const std::string& p) {
        auto b = std::filesystem::path(p);
        auto pc = b.replace_extension(".ppm").string();
        auto pa = (b.parent_path() / std::filesystem::path(b.stem().string() + "_alpha.ppm")).string();
        return loadpxm<uint8_t>(cs, pc) && loadpxm<uint8_t>(as, pa, false);
    }

    // Evaluate the texture on the given pixel coordinate
    Vec3 eval(Vec2 t) const {
        const auto u = t.x - floor(t.x);
        const auto v = t.y - floor(t.y);
        const int x = std::clamp(int(u * w), 0, w - 1);
        const int y = std::clamp(int(v * h), 0, h - 1);
        const int i = w * y + x;
        return { cs[3*i], cs[3*i+1], cs[3*i+2] };
    }

    Vec3 evalByPixelCoords(int x, int y) const {
        const int i = w * y + x;
        return { cs[3*i], cs[3*i+1], cs[3*i+2] };
    }

    Float evalAlpha(Vec2 t) const {
        const auto u = t.x - floor(t.x);
        const auto v = t.y - floor(t.y);
        const int x = std::clamp(int(u * w), 0, w - 1);
        const int y = std::clamp(int(v * h), 0, h - 1);
        const int i = w * y + x;
        return as[3*i];
    }

    bool hasAlpha() const {
        return !as.empty();
    }
};

class Texture_Bitmap final : public Texture {
private:
    Bitmap_PXM bitmap_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(bitmap_);
    }

public:
    virtual TextureSize size() const override {
        return { bitmap_.w, bitmap_.h };
    }

    virtual bool construct(const Json& prop) override {
        // Check extension
        const std::string path = prop["path"];
        const auto ext = std::filesystem::path(path).extension().string();
        if (ext == ".pfm") {
            return bitmap_.loadpfm(path);
        }
        else if (ext == ".ppm") {
            return bitmap_.loadppm(path);
        }
        LM_ERROR("Invalid extension [ext='{}']", ext);
        return false;
    }

    virtual Vec3 eval(Vec2 t) const override {
        return bitmap_.eval(t);
    }

    virtual Vec3 evalByPixelCoords(int x, int y) const override {
        return bitmap_.evalByPixelCoords(x, y);
    }

    virtual Float evalAlpha(Vec2 t) const override {
        return bitmap_.evalAlpha(t);
    }

    virtual bool hasAlpha() const override {
        return bitmap_.hasAlpha();
    }

    virtual TextureBuffer buffer() override {
        return { bitmap_.w, bitmap_.h, bitmap_.cs.data() };
    }
};
#endif

class Texture_Bitmap final : public Texture {
private:
    int w_;     // Width of the image
    int h_;     // Height of the image
    int c_;     // Number of components
    std::vector<float> data_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(w_, h_, data_);
    }

public:
    virtual TextureSize size() const override {
        return { w_, h_ };
    }

    virtual bool construct(const Json& prop) override {
        // Image path
        const std::string path = prop["path"];

        // Load as HDR image
        // LDR image is internally converted to HDR
        float* data = stbi_loadf(path.c_str(), &w_, &h_, &c_, 0);
        if (data == nullptr) {
            LM_ERROR("Failed to load image: {}", stbi_failure_reason());
            return false;
        }
        // Allocate and copy the data
        data_.assign(data, data + (w_*h_*c_));
        stbi_image_free(data);

        return false;
    }

    virtual Vec3 eval(Vec2 t) const override {
        const auto u = t.x - floor(t.x);
        const auto v = t.y - floor(t.y);
        const int x = std::clamp(int(u * w_), 0, w_ - 1);
        const int y = std::clamp(int(v * h_), 0, h_ - 1);
        const int i = w_ * y + x;
        return Vec3(data_[c_*i], data_[c_*i+1], data_[c_*i+2]);
    }

    virtual Vec3 evalByPixelCoords(int x, int y) const override {
        const int i = w_*y + x;
        return Vec3(data_[c_*i], data_[c_*i+1], data_[c_*i+2]);
    }

    virtual Float evalAlpha(Vec2 t) const override {
        const auto u = t.x - floor(t.x);
        const auto v = t.y - floor(t.y);
        const int x = std::clamp(int(u * w_), 0, w_ - 1);
        const int y = std::clamp(int(v * h_), 0, h_ - 1);
        const int i = w_ * y + x;
        return data_[c_*i+3];
    }

    virtual bool hasAlpha() const override {
        return c_ == 4;
    }

    virtual TextureBuffer buffer() override {
        return { w_, h_, c_, data_.data() };
    }
};

LM_COMP_REG_IMPL(Texture_Bitmap, "texture::bitmap");

LM_NAMESPACE_END(LM_NAMESPACE)
