/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
    Author of this file : Herveau Killian
*/

#pragma once

#include <lm/component.h>
#include <lm/json.h>

#if LM_COMPILER_MSVC
    #ifdef LM_TUNER_EXPORTS
        #define LM_TUNER_PUBLIC_API __declspec(dllexport)
    #else
        #define LM_TUNER_PUBLIC_API __declspec(dllimport)
    #endif
#elif LM_COMPILER_GCC || LM_COMPILER_CLANG
    #ifdef LM_TUNER_EXPORTS
        #define LM_TUNER_PUBLIC_API __attribute__ ((visibility("default")))
    #else
        #define LM_TUNER_PUBLIC_API
    #endif
#endif

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

/*!
    \addtogroup tuning
    @{
*/

/*!
    \brief autoTuner
*/
class Tuner : public Component {
public:
    /*!
        \brief sets the parameters of the tuner
    */

    virtual Json feedback(Float fb) = 0;
    virtual Json getConf() = 0;
};

/*!
    @}
*/

LM_NAMESPACE_END(LM_NAMESPACE)