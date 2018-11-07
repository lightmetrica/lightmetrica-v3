/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/parallel_context.h>
#include <lm/logger.h>
#include <omp.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel::detail)

class ParallelContext_OpenMP final : public ParallelContext {
private:
    int numThreads_;

public:
    virtual bool construct(const Json& prop) override {
        LM_INFO("Initializing parallel context [name='openmp']");
        LM_INDENT();
        if (prop.find("numThreads") == prop.end()) {
            numThreads_ = std::thread::hardware_concurrency();
        }
        else {
            numThreads_ = prop["numThreads"];
            if (numThreads_ <= 0) {
                numThreads_ = std::thread::hardware_concurrency() + numThreads_;
            }
        }
        omp_set_num_threads(numThreads_);
        LM_INFO("Number of threads: {}", numThreads_);
        return true;
    }

    virtual int numThreads() const override {
        return numThreads_;
    }

    virtual void foreach(long long numSamples, const ParallelProcessFunc& processFunc) const override {
        #pragma omp parallel for schedule(dynamic, 1)
        for (long long i = 0; i < numSamples; i++) {
            processFunc(i, omp_get_thread_num());
        }
    }
};

LM_COMP_REG_IMPL(ParallelContext_OpenMP, "parallel::openmp");

LM_NAMESPACE_END(LM_NAMESPACE::parallel::detail)
