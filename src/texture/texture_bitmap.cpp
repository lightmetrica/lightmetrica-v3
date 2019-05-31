/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/texture.h>
#include <lm/logger.h>
#include <lm/serial.h>
#include <lm/json.h>
#pragma warning(push)
#pragma warning(disable:4244) // possible loss of data
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#pragma warning(pop)

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

std::string sanitizeDirectorySeparator(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

class Texture_Bitmap final : public Texture {
private:
    int w_;     // Width of the image
    int h_;     // Height of the image
    int c_;     // Number of components
    std::vector<float> data_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(w_, h_, c_, data_);
    }

public:
    virtual TextureSize size() const override {
        return { w_, h_ };
    }

    virtual bool construct(const Json& prop) override {
        // Image path
        const std::string path = sanitizeDirectorySeparator(prop["path"]);
        LM_INFO("Loading texture [path='{}']", fs::path(path).filename().string());

        // Load as HDR image
        // LDR image is internally converted to HDR
        const bool flip = json::value<bool>(prop, "flip", true);
        stbi_set_flip_vertically_on_load(flip);
        float* data = stbi_loadf(path.c_str(), &w_, &h_, &c_, 0);
        if (data == nullptr) {
            LM_ERROR("Failed to load image: {} [path='{}']", stbi_failure_reason(), path);
            return false;
        }
        // Allocate and copy the data
        data_.assign(data, data + (w_*h_*c_));
        stbi_image_free(data);

        return true;
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
