/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/texture.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Texture_Bitmap final : public Texture {
public:
    virtual bool construct(const Json& prop) override {
        // TODO
        return true;
    }

};

LM_COMP_REG_IMPL(Texture_Bitmap, "texture::bitmap");

LM_NAMESPACE_END(LM_NAMESPACE)
