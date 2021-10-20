/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <lm/logger.h>
#include <lm/json.h>
#pragma warning(push)
#pragma warning(disable:4324)   // structure was padded due to alignment specifier
#include <embree3/rtcore.h>
#pragma warning(pop)
LM_NAMESPACE_BEGIN(LM_NAMESPACE)
    
static void handle_embree_error(void*, RTCError code, const char* str = nullptr) {
    if (code == RTC_ERROR_NONE) {
        return;
    }

    std::string codestr;
    switch (code) {
        case RTC_ERROR_UNKNOWN:           { codestr = "RTC_ERROR_UNKNOWN"; break; }
        case RTC_ERROR_INVALID_ARGUMENT:  { codestr = "RTC_ERROR_INVALID_ARGUMENT"; break; }
        case RTC_ERROR_INVALID_OPERATION: { codestr = "RTC_ERROR_INVALID_OPERATION"; break; }
        case RTC_ERROR_OUT_OF_MEMORY:     { codestr = "RTC_ERROR_OUT_OF_MEMORY"; break; }
        case RTC_ERROR_UNSUPPORTED_CPU:   { codestr = "RTC_ERROR_UNSUPPORTED_CPU"; break; }
        case RTC_ERROR_CANCELLED:         { codestr = "RTC_ERROR_CANCELLED"; break; }
        default:                          { codestr = "Invalid error code"; break; }
    }

    LM_ERROR("Embree error [code='{}']", codestr);
    if (str) {
        LM_INDENT();
        LM_ERROR(str);
    }

    LM_THROW_EXCEPTION(Error::None, codestr);
}

#if 0
inline std::string RTCtoStr(const RTCBuildArguments& rtc, const RTCSceneFlags& sf)
{
        std::string str = std::format(
        "\nbuildQuality:\t{}\n"
        "maxBranchingFactor:\t{}\n"
        "maxDepth:\t{}\n"
        "sahBlockSize:\t{}\n"
        "minLeafSize:\t{}\n"
        "maxLeafSize:\t{}\n"
        "traversalCost:\t{}\n"
        "intersectionCost:\t{}\n",
        "dynamic:\t{}\n"
        "compact:\t{}\n"
        "robust:\t{}\n"
        "filter:\t{}\n",
        rtc.buildQuality, rtc.maxBranchingFactor, rtc.maxDepth, rtc.sahBlockSize, rtc.minLeafSize,
        rtc.maxLeafSize, rtc.traversalCost, rtc.intersectionCost,
        sf & (1<<0),sf &(1<<1),sf&(1<<2),sf&(1<<3));
        return str;
    }
#endif
LM_NAMESPACE_END(LM_NAMESPACE)

LM_NAMESPACE_BEGIN(nlohmann)
template<>
struct adl_serializer<RTCSceneFlags> {
static void to_json(lm::Json &j, const RTCSceneFlags& sf)
{
    j = lm::Json(
        {
            {"dynamic",bool(sf & (1<<0))},
            {"compact",bool(sf & (1<<1))},
            {"robust", bool(sf & (1<<2))},
            {"filter", bool(sf & (1<<3))}
        }
    );
}
static void from_json(const lm::Json& j, RTCSceneFlags& sf)
{
    bool rtcsf[5] = {false};

    rtcsf[1] = lm::json::value<bool>(j,"dynamic",false);
    rtcsf[2] = lm::json::value<bool>(j,"compact",false);
    rtcsf[3] = lm::json::value<bool>(j,"robust",false);
    rtcsf[4] = lm::json::value<bool>(j,"filter",false);

    if(rtcsf[1])
        sf=RTC_SCENE_FLAG_DYNAMIC;
    if(rtcsf[2])
        sf= sf | RTC_SCENE_FLAG_COMPACT;
    if(rtcsf[3])
        sf= sf | RTC_SCENE_FLAG_ROBUST;
    if(rtcsf[4])
        sf= sf | RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION;
    if(!(rtcsf[0] || rtcsf[1] || rtcsf[2] || rtcsf[3])) // if no flag is set...
        sf=RTC_SCENE_FLAG_NONE;
}
};

template<>
struct adl_serializer<RTCBuildArguments> {
static void from_json(const lm::Json& j, RTCBuildArguments& rtc)
{
    rtc.buildQuality = RTCBuildQuality(lm::json::value<int>(j,"quality",1));
    rtc.maxDepth = lm::json::value<unsigned int>(j,"maxDepth",18);
    rtc.maxBranchingFactor = lm::json::value<unsigned int>(j,"maxBranchingFactor",2);
    rtc.sahBlockSize = lm::json::value<unsigned int>(j,"sahBlockSize",1);
    rtc.minLeafSize = lm::json::value<unsigned int>(j,"minLeafSize",1);
    rtc.maxLeafSize = lm::json::value<unsigned int>(j,"maxLeafSize",32);
    rtc.traversalCost = lm::json::value<float>(j,"travcost",1.0f);
    rtc.intersectionCost = lm::json::value<float>(j,"intcost",1.0f);
}
static void to_json(lm::Json &j, const RTCBuildArguments& rtc)
{
    j = lm::Json(
        {
            {"quality",rtc.buildQuality},
            {"maxDepth",rtc.maxDepth},
            {"maxBranchingFactor",rtc.maxBranchingFactor},
            {"sahBlockSize",rtc.sahBlockSize},
            {"minLeafSize",rtc.minLeafSize},
            {"maxLeafSize",rtc.maxLeafSize},
            {"travcost",rtc.traversalCost},
            {"intcost",rtc.intersectionCost},
        }
    );
}
};
LM_NAMESPACE_END(nlohmann)
