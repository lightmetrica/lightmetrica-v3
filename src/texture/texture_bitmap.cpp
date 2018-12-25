/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/texture.h>
#include <lm/logger.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

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

class Bitmap_PXM {
private:
    int w;                  // Width of the texture
    int h;                  // Height of the texture
    std::vector<Float> cs;  // Colors
    std::vector<Float> as;  // Alphas
    
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
        static std::vector<T> ct;
        FILE *f;
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
        double e;
        #if LM_COMPILER_MSVC
        fscanf_s(f, "%*s %d %d %lf%*c", &w, &h, &e);
        #else
        LM_UNUSED(fscanf(f, "%*s %d %d %lf%*c", &w, &h, &e));
        #endif
        const int sz = w * h * 3;
        ct.assign(sz, 0);
        c.assign(sz, 0);
        #if LM_COMPILER_MSVC
        fread_s(ct.data(), sz*sizeof(T), sizeof(T), sz, f);
        #else
        LM_UNUSED(fread(ct.data(), sizeof(T), sz, f));
        #endif
        for (int i = 0; i < sz; i++) {
            c[i] = postprocess(i, Float(e), ct);
        }
        std::fclose(f);
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
    virtual bool construct(const Json& prop) override {
        return bitmap_.loadppm(prop["path"]);
    }

    virtual Vec3 eval(Vec2 t) const override {
        return bitmap_.eval(t);
    }

    virtual Float evalAlpha(Vec2 t) const override {
        return bitmap_.evalAlpha(t);
    }

    virtual bool hasAlpha() const override {
        return bitmap_.hasAlpha();
    }
};

LM_COMP_REG_IMPL(Texture_Bitmap, "texture::bitmap");

LM_NAMESPACE_END(LM_NAMESPACE)
