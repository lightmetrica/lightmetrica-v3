/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/scheduler.h>
#include <lm/json.h>
#include <lm/parallel.h>
#include <lm/progress.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::scheduler)

// Sample-based SPPScheduler
class SPPScheduler_Samples : public SPPScheduler {
private:
    long long spp_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(spp_);
    }

public:
    virtual bool construct(const Json& prop) override {
        spp_ = json::value<long long>(prop, "spp");
        return true;
    }

    virtual long long run(long long numPixels, const ProcessFunc& process) const override {
        progress::ScopedReport progress_ctx_(numPixels * spp_);
        
        // Parallel loop for each pixel
        parallel::foreach(numPixels * spp_, [&](long long index, int threadid) {
            process(index / spp_, index % spp_, threadid);
        }, [&](long long processed) {
            progress::update(processed);
        });

        return spp_;
    }
};

LM_COMP_REG_IMPL(SPPScheduler_Samples, "sppscheduler::samples");

// ----------------------------------------------------------------------------

// Time-based SPPScheduler
class SPPScheduler_Time : public SPPScheduler {
private:
    double renderTime_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(renderTime_);
    }

public:
    virtual bool construct(const Json& prop) override {
        renderTime_ = json::value<Float>(prop, "render_time");
        return true;
    }
    
    virtual long long run(long long numPixels, const ProcessFunc& process) const override {
        progress::ScopedTimeReport progress_ctx_(renderTime_);
        
        const auto start = std::chrono::high_resolution_clock::now();
        long long spp = 0;
        while (true) {
            // Parallel loop for each pixel
            parallel::foreach(numPixels, [&](long long index, int threadid) {
                process(index, spp, threadid);
            }, [&](long long) {
                using namespace std::chrono;
                const auto now = high_resolution_clock::now();
                const auto elapsed = duration_cast<milliseconds>(now - start).count() / 1000.0;
                progress::updateTime(elapsed);
            });

            // Update processed spp
            spp++;

            // Check termination
            const auto curr = std::chrono::high_resolution_clock::now();
            const double elapsed = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(curr - start).count()) / 1000.0;
            if (elapsed > renderTime_) {
                break;
            }
        }

        return spp;
    }
};

LM_COMP_REG_IMPL(SPPScheduler_Time, "sppscheduler::time");

// ----------------------------------------------------------------------------

// Sample-based SPIScheduler
class SPIScheduler_Samples : public SPIScheduler {
private:
    long long numSamples_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(numSamples_);
    }

public:
    virtual bool construct(const Json& prop) override {
        numSamples_ = json::value<long long>(prop, "num_samples");
        return true;
    }

    virtual long long run(const ProcessFunc& process) const override {
        progress::ScopedReport progress_ctx_(numSamples_);
        parallel::foreach(numSamples_, [&](long long index, int threadid) {
            process(index, threadid);
        }, [&](long long processed) {
            progress::update(processed);
        });

        return numSamples_;
    }
};

LM_COMP_REG_IMPL(SPIScheduler_Samples, "spischeduler::samples");

// ----------------------------------------------------------------------------

// Time-based SPIScheduler
class SPIScheduler_Time : public SPIScheduler {
private:
    double renderTime_;
    long long samplesPerIter_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(renderTime_, samplesPerIter_);
    }

public:
    virtual bool construct(const Json& prop) override {
        renderTime_ = json::value<Float>(prop, "render_time");
        samplesPerIter_ = json::value<long long>(prop, "samples_per_iter", 10000000);
        return true;
    }

    virtual long long run(const ProcessFunc& process) const override {
        progress::ScopedTimeReport progress_ctx_(renderTime_);
        const auto start = std::chrono::high_resolution_clock::now();
        long long processed = 0;
        while (true) {
            // Parallel loop
            parallel::foreach(samplesPerIter_, [&](long long index, int threadid) {
                process(processed + index, threadid);
            }, [&](long long) {
                using namespace std::chrono;
                const auto now = high_resolution_clock::now();
                const auto elapsed = duration_cast<milliseconds>(now - start).count() / 1000.0;
                progress::updateTime(elapsed);
            });

            // Update processed samples
            processed += samplesPerIter_;

            // Check termination
            const auto curr = std::chrono::high_resolution_clock::now();
            const double elapsed = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(curr - start).count()) / 1000.0;
            if (elapsed > renderTime_) {
                break;
            }
        }
        return processed;
    }
};

LM_COMP_REG_IMPL(SPIScheduler_Time, "spischeduler::time");

LM_NAMESPACE_END(LM_NAMESPACE::scheduler)
