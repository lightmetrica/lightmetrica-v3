/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/parallel.h>
#include <lm/logger.h>
#include <lm/json.h>
#include <omp.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel::detail)

class ParallelContext_OpenMP final : public ParallelContext {
private:
    int numThreads_;

public:
    virtual bool construct(const Json& prop) override {
        LM_INFO("Initializing parallel context [name='openmp']");
        LM_INDENT();
        numThreads_ = valueOr(prop, "numThreads", std::thread::hardware_concurrency());
        if (numThreads_ <= 0) {
            numThreads_ = std::thread::hardware_concurrency() + numThreads_;
        }
        omp_set_num_threads(numThreads_);
        LM_INFO("Number of threads: {}", numThreads_);
        return true;
    }

    virtual int numThreads() const override {
        return numThreads_;
    }

    virtual void foreach(long long numSamples, const ParallelProcessFunc& processFunc) const override {
        using namespace std::literals::chrono_literals;

        // Processed number of samples
        std::atomic<long long> processed = 0;

        // Last progress update time and number of samples
        const auto start = std::chrono::high_resolution_clock::now();
        auto lastUpdated = start;
        long long lastProcessed = 0;

        // Execute parallel loop
        #pragma omp parallel for schedule(dynamic, 1)
        for (long long i = 0; i < numSamples; i++) {
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
                const auto now = std::chrono::high_resolution_clock::now();
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdated);
                if (elapsed > .5s) {
                    // Current processed number of samples
                    const long long currProcessed = processed;

                    // Compute ETA
                    const auto eta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
                        * (numSamples - currProcessed) / currProcessed;

                    // Print progress
                    LM_PROGRESS("Processing [iter={}/{}, progress={:.1f}%, ETA={:.1f}s]",
                        currProcessed,
                        numSamples,
                        double(currProcessed) / numSamples * 100,
                        eta.count() / 1000.0);

                    lastUpdated = now;
                    lastProcessed = currProcessed;
                }
            }
        }
        LM_PROGRESS_END("Processing [completed]");
    }
};

LM_COMP_REG_IMPL(ParallelContext_OpenMP, "parallel::openmp");

LM_NAMESPACE_END(LM_NAMESPACE::parallel::detail)
