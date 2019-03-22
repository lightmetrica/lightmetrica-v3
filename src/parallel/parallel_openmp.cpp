/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/parallel.h>
#include <lm/logger.h>
#include <lm/json.h>
#include <lm/progress.h>
#include <omp.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel)

class ParallelContext_OpenMP final : public ParallelContext {
private:
    int numThreads_;

public:
    virtual bool construct(const Json& prop) override {
        numThreads_ = json::value(prop, "numThreads", std::thread::hardware_concurrency());
        if (numThreads_ <= 0) {
            numThreads_ = std::thread::hardware_concurrency() + numThreads_;
        }
        omp_set_num_threads(numThreads_);
        return true;
    }

    virtual int numThreads() const override {
        return numThreads_;
    }

    virtual bool mainThread() const override {
        return omp_get_thread_num() == 0;
    }

    virtual void foreach(long long numSamples, const ParallelProcessFunc& processFunc) const override {
        // Processed number of samples
        std::atomic<long long> processed = 0;

        // Captured exceptions inside the parallel loop
        std::atomic<bool> done = false;
        std::exception_ptr exp;
        std::mutex explock;

        // Execute parallel loop
        progress::ScopedReport progress_(numSamples);
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
                processFunc(i, threadId);

                // Update processed number of samples
                constexpr long long UpdateInterval = 100;
                if (thread_local long long count = 0; ++count >= UpdateInterval) {
                    processed += count;
                    count = 0;
                }

                // Update progress
                if (threadId == 0) {
                    progress::update(processed);
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
