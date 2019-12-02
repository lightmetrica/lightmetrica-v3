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
#include <lm/film.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::scheduler)

// Sample-based SPPScheduler
class Scheduler_SPP_Sample : public Scheduler {
private:
    long long spp_;
    Film* film_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(spp_, film_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
    }

public:
    virtual void construct(const Json& prop) override {
        spp_ = json::value<long long>(prop, "spp");
        film_ = json::comp_ref<Film>(prop, "output");
    }

    virtual long long run(const ProcessFunc& process) const override {
        const auto numPixels = film_->num_pixels();
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

LM_COMP_REG_IMPL(Scheduler_SPP_Sample, "scheduler::spp::sample");

// ------------------------------------------------------------------------------------------------

// Time-based SPPScheduler
class Scheduler_SPP_Time : public Scheduler {
private:
    double renderTime_;
    Film* film_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(renderTime_, film_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
    }

public:
    virtual void construct(const Json& prop) override {
        renderTime_ = json::value<Float>(prop, "render_time");
        film_ = json::comp_ref<Film>(prop, "output");
    }
    
    virtual long long run(const ProcessFunc& process) const override {
        const auto numPixels = film_->num_pixels();
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
                progress::update_time(elapsed);
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

LM_COMP_REG_IMPL(Scheduler_SPP_Time, "scheduler::spp::time");

// ------------------------------------------------------------------------------------------------

// Sample-based SPIScheduler
class Scheduler_SPI_Sample : public Scheduler {
private:
    long long numSamples_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(numSamples_);
    }
  
public:
    virtual void construct(const Json& prop) override {
        numSamples_ = json::value<long long>(prop, "num_samples");
    }

    virtual long long run(const ProcessFunc& process) const override {
        progress::ScopedReport progress_ctx_(numSamples_);
        parallel::foreach(numSamples_, [&](long long index, int threadid) {
            process(0, index, threadid);
        }, [&](long long processed) {
            progress::update(processed);
        });

        return numSamples_;
    }
};

LM_COMP_REG_IMPL(Scheduler_SPI_Sample, "scheduler::spi::sample");

// ------------------------------------------------------------------------------------------------

// Time-based SPIScheduler
class Scheduler_SPI_Time : public Scheduler {
private:
    double renderTime_;
    long long samplesPerIter_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(renderTime_, samplesPerIter_);
    }

public:
    virtual void construct(const Json& prop) override {
        renderTime_ = json::value<Float>(prop, "render_time");
        samplesPerIter_ = json::value<long long>(prop, "samples_per_iter", 1000000);
    }

    virtual long long run(const ProcessFunc& process) const override {
        progress::ScopedTimeReport progress_ctx_(renderTime_);
        const auto start = std::chrono::high_resolution_clock::now();
        long long processed = 0;
        while (true) {
            // Parallel loop
            parallel::foreach(samplesPerIter_, [&](long long index, int threadid) {
                process(0, processed + index, threadid);
            }, [&](long long) {
                using namespace std::chrono;
                const auto now = high_resolution_clock::now();
                const auto elapsed = duration_cast<milliseconds>(now - start).count() / 1000.0;
                progress::update_time(elapsed);
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

LM_COMP_REG_IMPL(Scheduler_SPI_Time, "scheduler::spi::time");

LM_NAMESPACE_END(LM_NAMESPACE::scheduler)
