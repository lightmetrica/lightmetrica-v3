/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/progress.h>
#include <lm/logger.h>

using namespace std::chrono;
using namespace std::literals::chrono_literals;

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::progress::detail)

// Multiplexed progress reporter
class ProgressContext_Mux : public ProgressContext {
private:
    std::vector<Ptr<ProgressContext>> ctx_;

public:
    virtual bool construct(const Json& prop) override {
        for (const auto& entry : prop) {
            auto it = entry.begin();
            auto ctx = comp::create<ProgressContext>(it.key(), "", it.value());
            if (!ctx) {
                return false;
            }
            ctx_.push_back(std::move(ctx));
        }
        return true;
    }

    virtual void start(long long total) override {
        for (auto& ctx : ctx_) { ctx->start(total); }
    }

    virtual void update(long long processed) override {
        for (auto& ctx : ctx_) { ctx->update(processed); }
    }

    virtual void end() override {
        for (auto& ctx : ctx_) { ctx->end(); }
    }
};

LM_COMP_REG_IMPL(ProgressContext_Mux, "progress::mux");

// ----------------------------------------------------------------------------

// Delay the update for the specified seconds
class ProgressContext_Delay : public ProgressContext {
private:
    milliseconds delay_;
    Ptr<ProgressContext> ctx_;                       // Underlying progress context
    time_point<high_resolution_clock> start_;        // Time starting progress report
    time_point<high_resolution_clock> lastUpdated_;  // Last updated time

public:
    virtual bool construct(const Json& prop) override {
        delay_ = milliseconds(prop["delay"].get<int>());
        auto it = prop["progress"].begin();
        ctx_ = comp::create<ProgressContext>(it.key(), "", it.value());
        return true;
    }

    virtual void start(long long total) override {
        start_ = high_resolution_clock::now();
        lastUpdated_ = start_;
        ctx_->start(total);
    }

    virtual void update(long long processed) override {
        const auto now = high_resolution_clock::now();
        const auto elapsed = duration_cast<milliseconds>(now - lastUpdated_);
        if (elapsed > delay_) {
            ctx_->update(processed);
            lastUpdated_ = now;
        }
    }

    virtual void end() override {
        ctx_->end();
    }
};

LM_COMP_REG_IMPL(ProgressContext_Delay, "progress::delay");

// ----------------------------------------------------------------------------

// Default progress reporter
class ProgressContext_Default : public ProgressContext {
private:
    long long total_;                                // Total number of progress updates
    time_point<high_resolution_clock> start_;        // Time starting progress report
    time_point<high_resolution_clock> lastUpdated_;  // Last updated time
    
public:
    virtual void start(long long total) override {
        total_ = total;
        start_ = high_resolution_clock::now();
        lastUpdated_ = start_;
    }

    virtual void update(long long processed) override {
        const auto now = high_resolution_clock::now();
        const auto elapsed = duration_cast<milliseconds>(now - lastUpdated_);
        if (elapsed > .5s) {
            // Compute ETA
            const auto etaStr = [&]() -> std::string {
                if (processed == 0) {
                    return "";
                }
                const auto eta = duration_cast<milliseconds>(now - start_)
                    * (total_ - processed) / processed;
                return fmt::format(", ETA={:.1f}s", eta.count() / 1000.0);
            }();

            // Print progress
            LM_PROGRESS("Processing [iter={}/{}, progress={:.1f}%{}]",
                processed,
                total_,
                double(processed) / total_ * 100,
                etaStr);

            lastUpdated_ = now;
        }
    }

    virtual void end() override {
        LM_PROGRESS_END("Processing [completed]");
    }
};

LM_COMP_REG_IMPL(ProgressContext_Default, "progress::default");

LM_NAMESPACE_END(LM_NAMESPACE::progress::detail)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::progress)

using Instance = comp::detail::ContextInstance<detail::ProgressContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void start(long long total) {
    Instance::get().start(total);
}

LM_PUBLIC_API void update(long long processed) {
    Instance::get().update(processed);
}

LM_PUBLIC_API void end() {
    Instance::get().end();
}

LM_NAMESPACE_END(LM_NAMESPACE::progress)
