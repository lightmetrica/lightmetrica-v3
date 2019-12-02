/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/progresscontext.h>
#include <lm/logger.h>

using namespace std::chrono;
using namespace std::literals::chrono_literals;

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::progress)

// Multiplexed progress reporter
class ProgressContext_Mux : public ProgressContext {
private:
    std::vector<Ptr<ProgressContext>> ctx_;

public:
    virtual void construct(const Json& prop) override {
        for (const auto& entry : prop) {
            auto it = entry.begin();
            auto ctx = comp::create<ProgressContext>(it.key(), "", it.value());
            if (!ctx) {
                LM_THROW_EXCEPTION_DEFAULT(Error::InvalidArgument);
            }
            ctx_.push_back(std::move(ctx));
        }
    }

    virtual void start(ProgressMode mode, long long total, double totalTime) override {
        for (auto& ctx : ctx_) { ctx->start(mode, total, totalTime); }
    }

    virtual void update(long long processed) override {
        for (auto& ctx : ctx_) { ctx->update(processed); }
    }

    virtual void update_time(Float elapsed) override {
        for (auto& ctx : ctx_) { ctx->update_time(elapsed); }
    }

    virtual void end() override {
        for (auto& ctx : ctx_) { ctx->end(); }
    }
};

LM_COMP_REG_IMPL(ProgressContext_Mux, "progress::mux");

// ------------------------------------------------------------------------------------------------

// Delay the update for the specified seconds
class ProgressContext_Delay : public ProgressContext {
private:
    milliseconds delay_;
    Ptr<ProgressContext> ctx_;                       // Underlying progress context
    time_point<high_resolution_clock> start_;        // Time starting progress report
    time_point<high_resolution_clock> lastUpdated_;  // Last updated time

public:
    virtual void construct(const Json& prop) override {
        delay_ = milliseconds(json::value<int>(prop, "delay"));
        auto it = prop["progress"].begin();
        ctx_ = comp::create<ProgressContext>(it.key(), "", it.value());
    }

    virtual void start(ProgressMode mode, long long total, double totalTime) override {
        start_ = high_resolution_clock::now();
        lastUpdated_ = start_;
        ctx_->start(mode, total, totalTime);
    }

    virtual void update(long long processed) override {
        const auto now = high_resolution_clock::now();
        const auto elapsed = duration_cast<milliseconds>(now - lastUpdated_);
        if (elapsed > delay_) {
            ctx_->update(processed);
            lastUpdated_ = now;
        }
    }

    virtual void update_time(Float elapsed_) override {
        const auto now = high_resolution_clock::now();
        const auto elapsed = duration_cast<milliseconds>(now - lastUpdated_);
        if (elapsed > delay_) {
            ctx_->update_time(elapsed_);
            lastUpdated_ = now;
        }
    }

    virtual void end() override {
        ctx_->end();
    }
};

LM_COMP_REG_IMPL(ProgressContext_Delay, "progress::delay");

// ------------------------------------------------------------------------------------------------

// Default progress reporter
class ProgressContext_Default : public ProgressContext {
private:
    ProgressMode mode_;								 // Progress reporting mode
    long long total_;                                // Total number of progress updates (used in Samples mode)
    double totalTime_;								 // Total duration (used in Time mode)
    time_point<high_resolution_clock> start_;        // Time starting progress report
    time_point<high_resolution_clock> lastUpdated_;  // Last updated time
    
public:
    virtual void start(ProgressMode mode, long long total, double totalTime) override {
        mode_ = mode;
        total_ = total;
        totalTime_ = totalTime;
        start_ = high_resolution_clock::now();
        lastUpdated_ = start_;
    }

    virtual void update(long long processed) override {
        if (mode_ != ProgressMode::Samples) {
            return;
        }
        const auto now = high_resolution_clock::now();
        const auto elapsedFromLastUpdate = duration_cast<milliseconds>(now - lastUpdated_);
        if (elapsedFromLastUpdate > .5s) {
            // Estimate ETA
            const auto etaStr = [&]() -> std::string {
                if (processed == 0) {
                    return "";
                }
                const auto eta = duration_cast<milliseconds>(now - start_) * (total_ - processed) / processed;
                return fmt::format(", ETA={:.1f}s", eta.count() / 1000.0);

            }();
            LM_PROGRESS("Processing [iter={}/{}, progress={:.1f}%{}]",
                processed,
                total_,
                double(processed) / total_ * 100,
                etaStr);
            lastUpdated_ = now;
        }
    }

    virtual void update_time(Float elapsed) override {
        if (mode_ != ProgressMode::Time) {
            return;
        }
        const auto now = high_resolution_clock::now();
        const auto elapsedFromLastUpdate = duration_cast<milliseconds>(now - lastUpdated_);
        if (elapsedFromLastUpdate > .5s) {
            const auto eta = totalTime_ - elapsed;
            const auto etaStr = fmt::format(", ETA={:.1f}s", eta);
            LM_PROGRESS("Processing [iter={}/{}, progress={:.1f}%{}]",
                elapsed,
                totalTime_,
                elapsed / totalTime_ * 100,
                etaStr);
            lastUpdated_ = now;
        }
    }

    virtual void end() override {
        LM_PROGRESS_END("Processing [completed]");
    }

private:
    // Helper function to compute ETA
    template <typename T>
    double estimateETA(time_point<high_resolution_clock> now, T total, T processed) const {
        const auto eta = duration_cast<milliseconds>(now - start_)
            * (total - processed) / processed;
        return eta.count() / 1000.0;
    }
};

LM_COMP_REG_IMPL(ProgressContext_Default, "progress::default");

// ------------------------------------------------------------------------------------------------

using Instance = comp::detail::ContextInstance<ProgressContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void start(ProgressMode mode, long long total, double totalTime) {
    Instance::get().start(mode, total, totalTime);
}

LM_PUBLIC_API void update(long long processed) {
    Instance::get().update(processed);
}

LM_PUBLIC_API void update_time(Float elapsed) {
    Instance::get().update_time(elapsed);
}

LM_PUBLIC_API void end() {
    Instance::get().end();
}

LM_NAMESPACE_END(LM_NAMESPACE::progress)
