/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include <chrono>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)
LM_NAMESPACE_BEGIN(timer)

/*!
    \addtogroup timer
    @{
*/

/*!
    \brief Scoped Timer for elapsed time

    \rst
    This class provides a compact way to time code using the RAII idiom:
    Resource Acquisition Is Initialization
    \endrst
*/
class ScopedTimer : public Component {
private:
    std::chrono::high_resolution_clock::time_point start;
public:
    /*!
        \brief Timer initialisation at the time of object creation
    */
    ScopedTimer():start(std::chrono::high_resolution_clock::now()){}

    /*!
        \brief elapsed time since object creation in seconds
    */
   virtual Float now() const{
       return std::chrono::duration<Float>(std::chrono::high_resolution_clock::now()-start).count();
   }
};

/*!
    @}
*/

LM_NAMESPACE_END(timer)
LM_NAMESPACE_END(LM_NAMESPACE)
