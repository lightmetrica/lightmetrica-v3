/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/api.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::api)

// ----------------------------------------------------------------------------

/*
    User API context.
    Manages all global states manipulated by user apis.
*/
class UserAPIContext {
private:
    
    

};

// ----------------------------------------------------------------------------

LM_PUBLIC_API void init()
{

}

LM_PUBLIC_API void shutdown()
{

}

LM_PUBLIC_API int primitive(const std::string& name)
{
    return 42;
}

LM_NAMESPACE_END(LM_NAMESPACE::api)
