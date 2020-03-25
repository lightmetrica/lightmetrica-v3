#include "embree.h"
#include <lm/json.h>
LM_NAMESPACE_BEGIN(LM_NAMESPACE)

#define tryAt(j,str,v) try{j.at(str).get_to(v);}catch(...){ v=v;}

    void to_json(lm::Json &j, const RTCSceneFlags& sf)
    {
        j= lm::Json(
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
        bool rtcsf[5]= {false};

        tryAt(j,"dynamic",rtcsf[1]);
        tryAt(j,"compact",rtcsf[2]);
        tryAt(j,"robust",rtcsf[3]);
        tryAt(j,"filter",rtcsf[4]);
        
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
    
void from_json(const lm::Json& j, RTCBuildArguments& rtc)
    {
        tryAt(j,"quality",rtc.buildQuality);
        tryAt(j,"maxDepth",rtc.maxDepth);
        tryAt(j,"maxBranchingFactor",rtc.maxBranchingFactor);
        tryAt(j,"sahBlockSize",rtc.sahBlockSize);
        tryAt(j,"minLeafSize",rtc.minLeafSize);
        tryAt(j,"maxLeafSize",rtc.maxLeafSize);
        tryAt(j,"travcost",rtc.traversalCost);
        tryAt(j,"intcost",rtc.intersectionCost);
    }
void to_json(lm::Json &j, const RTCBuildArguments& rtc)
    {
        j= lm::Json(
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

LM_NAMESPACE_END(LM_NAMESPACE)