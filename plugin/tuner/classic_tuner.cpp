
/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
    Author of this file : Herveau Killian
*/
#include "tuner.h"
#include <lm/logger.h>
#include <lm/json.h>
#include <Tuning/Tuner.h>

#include <initializer_list>

//#define CTUNER_IN_DEV

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

class Classic_Tuner final : public Tuner
{
    std::unique_ptr<Tuning::ContainerTuner<Tuning::DefaultSearch>> tuner;
    Json tuningConfig;
    long int iter;

    virtual Json getConf() {    return tuningConfig; }
    template <typename T> //UNBOUNDED
    void addUnbounded(const Json &prop, T &value)
    {
        tuner->addParameter(prop["name"].get<std::string>(), value);
    }

    template <typename T> //BOUNDED
    void addBounded(const Json &prop, T &value)
    {
        tuner->addParameter(prop["name"].get<std::string>(), value, prop["min"].get<T>(), prop["max"].get<T>());
    }

    template <typename T> //NOMINAL
    void addNominal(const Json &prop, T &value)
    {
        tuner->addParameter(prop["name"].get<std::string>(), value, prop["possibleValues"].get<std::vector<T>>());
    }

    template <typename T> //NOMINAL
    void makeParameters(Json &prop, bool nominal, bool hasMin)
    {
        auto &hold = *(prop["value"].get_ptr<T *>());
#ifdef CTUNER_IN_DEV
        LM_INFO("param : {} : {}", prop["name"].get<std::string>(), hold);
#endif
        if (nominal)
        {
            addNominal<T>(prop, hold);
#ifdef CTUNER_IN_DEV
            LM_INFO("ADDED NOMINAL");
#endif
        }
        else if (hasMin)
        {
            addBounded<T>(prop, hold);
#ifdef CTUNER_IN_DEV
            LM_INFO("ADDED BOUNDED");
#endif
        }
        else
        {
            addUnbounded<T>(prop, hold);
#ifdef CTUNER_IN_DEV
            LM_INFO("ADDED UNBOUNDED");
#endif
        }
    }

    void prepJsonParameter(Json &prop, bool nominal, bool hasMin)
    {

        switch (prop["value"].type())
        {

        case nlohmann::detail::value_t::number_integer:
        {
            makeParameters<nlohmann::json::number_integer_t>(prop, nominal, hasMin);
            break;
        }
        case nlohmann::detail::value_t::number_unsigned:
        {
            makeParameters<nlohmann::json::number_unsigned_t>(prop, nominal, hasMin);
            break;
        }
        case nlohmann::detail::value_t::number_float:
        {
            makeParameters<nlohmann::json::number_float_t>(prop, nominal, hasMin);
            break;
        }
        case nlohmann::detail::value_t::boolean:
        {
            makeParameters<nlohmann::json::boolean_t>(prop, nominal, hasMin);
            break;
        }
            /* not implemented in libtuning
        case nlohmann::detail::value_t::string:
        { //can only be nominal
            LM_INFO("ADDED NOMINAL");
            makeParameters<nlohmann::json::string_t>(prop, nominal, hasMin);
            break;
        }*/
        }
    }

    void setParameters()
    {
        for (auto &param : *(tuningConfig["parameters"].get_ptr<nlohmann::json::array_t *>()))
        { //must have possibleValues or min and max or nothing of these three but not just min or just max
            bool nominal = false;
            bool hasMin = false;

            try
            {
                param.at("possibleValues");
                nominal = true;
            }
            catch (nlohmann::json::exception &e)
            {   
                nominal = false;
            }
            if (!nominal)
            {
                try
                {
                    param.at("min");
                    hasMin = true;
                }
                catch (nlohmann::json::exception &e)
                {
                    hasMin = false;
                }
            }
            prepJsonParameter(param, nominal, hasMin);
        }
    }

    virtual void construct(const Json &prop) override
    {
#ifdef CTUNER_IN_DEV
        LM_INFO(" ----- BEGIN TUNER BUILD ----- ");
#endif
        iter = 0;
        tuner.reset(new Tuning::ContainerTuner<Tuning::DefaultSearch>("classic"));
        tuner->getOptions().setIgnoreNominal(true);
        tuner->getOptions().setEnableSSG(false);
        tuningConfig = prop;
        setParameters();
#ifdef CTUNER_IN_DEV
        LM_INFO(" ----- END TUNER BUILD ----- ");
#endif
    }

    virtual Json feedback(Float fb) override
    {
        tuner->feedback(static_cast<float>(fb));
        tuner->next();
#ifdef CTUNER_IN_DEV
        LM_INFO(" ----- BEGIN FEEDBACK ----- ");
        for (auto &param : tuningConfig["parameters"].get<nlohmann::json::array_t>())
        {
            LM_INFO("param : {}", param["name"].get<std::string>());

            switch (param["value"].type())
            {
            case nlohmann::detail::value_t::number_integer:
            {
                LM_INFO("value : {}", param["value"].get<nlohmann::json::number_integer_t>());
                break;
            }
            case nlohmann::detail::value_t::number_unsigned:
            {
                LM_INFO("value : {}", param["value"].get<nlohmann::json::number_unsigned_t>());
                break;
            }
            case nlohmann::detail::value_t::number_float:
            {
                LM_INFO("value : {}", param["value"].get<nlohmann::json::number_float_t>());
                break;
            }
            }
        }
        LM_INFO(" ----- END FEEDBACK ----- ");
#endif
        iter++;
        return tuningConfig;
    }
};

LM_COMP_REG_IMPL(Classic_Tuner, "tuner::classic");

LM_NAMESPACE_END(LM_NAMESPACE)