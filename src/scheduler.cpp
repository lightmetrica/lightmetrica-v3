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
    double render_time_;
    Film* film_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(render_time_, film_);
    }

    virtual void foreach_underlying(const ComponentVisitor& visit) override {
        comp::visit(visit, film_);
    }

public:
    virtual void construct(const Json& prop) override {
        render_time_ = json::value<Float>(prop, "render_time");
        film_ = json::comp_ref<Film>(prop, "output");
    }
    
    virtual long long run(const ProcessFunc& process) const override {
        const auto numPixels = film_->num_pixels();
        progress::ScopedTimeReport progress_ctx_(render_time_);
        
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
            if (elapsed > render_time_) {
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
    long long num_samples_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(num_samples_);
    }
  
public:
    virtual void construct(const Json& prop) override {
        num_samples_ = json::value<long long>(prop, "num_samples");
    }

    virtual long long run(const ProcessFunc& process) const override {
        progress::ScopedReport progress_ctx_(num_samples_);
        parallel::foreach(num_samples_, [&](long long index, int threadid) {
            process(0, index, threadid);
        }, [&](long long processed) {
            progress::update(processed);
        });

        return num_samples_;
    }
};

LM_COMP_REG_IMPL(Scheduler_SPI_Sample, "scheduler::spi::sample");

// ------------------------------------------------------------------------------------------------

// Time-based SPIScheduler
class Scheduler_SPI_Time : public Scheduler {
private:
    double render_time_;
    long long samples_per_iter_;

public:
    LM_SERIALIZE_IMPL(ar) {
        ar(render_time_, samples_per_iter_);
    }

public:
    virtual void construct(const Json& prop) override {
        render_time_ = json::value<Float>(prop, "render_time");
        samples_per_iter_ = json::value<long long>(prop, "samples_per_iter", 100000);
    }

    virtual long long run(const ProcessFunc& process) const override {
        progress::ScopedTimeReport progress_ctx_(render_time_);
        const auto start = std::chrono::high_resolution_clock::now();
        long long processed = 0;
        while (true) {
            // Parallel loop
            parallel::foreach(samples_per_iter_, [&](long long index, int threadid) {
                process(0, processed + index, threadid);
            }, [&](long long) {
                using namespace std::chrono;
                const auto now = high_resolution_clock::now();
                const auto elapsed = duration_cast<milliseconds>(now - start).count() / 1000.0;
                progress::update_time(elapsed);
            });

            // Update processed samples
            processed += samples_per_iter_;

            // Check termination
            const auto curr = std::chrono::high_resolution_clock::now();
            const double elapsed = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(curr - start).count()) / 1000.0;
            if (elapsed > render_time_) {
                break;
            }
        }
        return processed;
    }
};

LM_COMP_REG_IMPL(Scheduler_SPI_Time, "scheduler::spi::time");

LM_NAMESPACE_END(LM_NAMESPACE::scheduler)
