/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include "../pch.h"
#include <lm/renderer.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Renderer_Blank final : public Renderer {
public:
    virtual void render(const Scene& scene) const override {
        
    }
};

LM_COMP_REG_IMPL(Renderer_Blank, "renderer::blank");

LM_NAMESPACE_END(LM_NAMESPACE)
