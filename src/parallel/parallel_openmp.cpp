/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/parallelcontext.h>
#include <lm/progress.h>
#include <omp.h>
#if LM_PLATFORM_WINDOWS
#include <Windows.h>
#endif

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel)

class ParallelContext_OpenMP final : public ParallelContext {
private:
    long long progressUpdateInterval_;	// Number of samples per progress update
    int numThreads_;					// Number of threads

public:
    virtual void construct(const Json& prop) override {
        progressUpdateInterval_ = json::value<long long>(prop, "progress_update_interval", 100);
        numThreads_ = json::value(prop, "numThreads", std::thread::hardware_concurrency());
        if (numThreads_ <= 0) {
            numThreads_ = std::thread::hardware_concurrency() + numThreads_;
        }
        omp_set_num_threads(numThreads_);
    }

    virtual int num_threads() const override {
        return numThreads_;
    }

    virtual bool main_thread() const override {
        return omp_get_thread_num() == 0;
    }

    virtual void foreach(long long numSamples, const ParallelProcessFunc& processFunc, const ProgressUpdateFunc& progressUpdateFunc) const override {
        // Captured exceptions inside the parallel loop
        std::atomic<bool> done = false;
        std::exception_ptr exp;
        std::mutex explock;

        // Execute parallel loop
        std::atomic<long long> processed = 0;
        #pragma omp parallel for schedule(dynamic, 1)
        for (long long i = 0; i < numSamples; i++) {
            // Spin the loop if cancellation is requested
            if (done) {
                continue;
            }

            // OpenMP prohibits to throw exception inside parallel region
            // and to catch in the outer context.
            // cf. p.10
            // https://www.openmp.org/wp-content/uploads/cspec20_bars.pdf
            // A throw executed inside a parallel region must cause execution to resume within
            // the dynamic extent of the same structured block, and it must be caught by the
            // same thread that threw the exception.
            try {
                const int threadId = omp_get_thread_num();

                #if LM_PLATFORM_WINDOWS
                // Set process group
                GROUP_AFFINITY mask;
                if (GetNumaNodeProcessorMaskEx(threadId % 2, &mask)) {
                    SetThreadGroupAffinity(GetCurrentThread(), &mask, nullptr);
                }
                #endif

                // Dispatch user-defined process
                processFunc(i, threadId);

                // Update processed number of samples
                if (thread_local long long count = 0; ++count >= progressUpdateInterval_) {
                    processed += count;
                    count = 0;
                }

                // Update progress
                if (threadId == 0) {
                    progressUpdateFunc(processed);
                }
            }
            catch (...) {
                // Capture exception
                // pick the last one if some of the threads throw exceptions simultaneously
                std::unique_lock<std::mutex> lock(explock);
                exp = std::current_exception();
                done = true;
            }
        }
        
        // Rethrow exception if available
        if (exp) {
            std::rethrow_exception(exp);
        }
    }
};

LM_COMP_REG_IMPL(ParallelContext_OpenMP, "parallel::openmp");

LM_NAMESPACE_END(LM_NAMESPACE::parallel)
