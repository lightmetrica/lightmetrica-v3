#include "embree.h"
#include <lm/json.h>

LM_NAMESPACE_BEGIN(nlohmann)
template<>
struct adl_serializer<RTCSceneFlags> {
void to_json(lm::Json &j, const RTCSceneFlags& sf)
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
void from_json(const lm::Json& j, RTCSceneFlags& sf)
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
void from_json(const lm::Json& j, RTCBuildArguments& rtc)
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
void to_json(lm::Json &j, const RTCBuildArguments& rtc)
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
