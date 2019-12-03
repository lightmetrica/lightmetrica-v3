/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/parallelcontext.h>
#include <lm/distributed.h>
#include <lm/json.h>
#include <zmq.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel)

class ParallelContext_DistMaster final : public ParallelContext {
public:
    virtual int num_threads() const {
        return 0;
    }

    virtual bool main_thread() const {
        return true;
    }

    virtual void foreach(long long num_samples, const ParallelProcessFunc&, const ProgressUpdateFunc& progress_update_func) const override {
        std::mutex mut;
        std::condition_variable cond;
        long long total_processed = 0;

        // Called when a task is finished
        distributed::master::on_worker_task_finished([&](long long processed) {
            std::unique_lock<std::mutex> lock(mut);
            total_processed += processed;
            cond.notify_one();
        });

        // Execute tasks
        const long long WorkSize = 10000;
        const long long Iter = (num_samples + WorkSize - 1) / WorkSize;
        for (long long i = 0; i < Iter; i++) {
            const long long start = i * WorkSize;
            const long long end = std::min((i + 1) * WorkSize, num_samples);
            distributed::master::process_worker_task(start, end);
        }

        // Wait for completion
        std::unique_lock<std::mutex> lock(mut);
        cond.wait(lock, [&] {
			progress_update_func(total_processed);
            return total_processed == num_samples;
        });

        // Notify process has completed
        distributed::master::notify_process_completed();
    }
};

LM_COMP_REG_IMPL(ParallelContext_DistMaster, "parallel::distributed_master");

// ------------------------------------------------------------------------------------------------

class ParallelContext_DistWorker final : public ParallelContext {
public:
    Ptr<ParallelContext> local_context_;

public:
    virtual void construct(const Json& prop) override {
        local_context_ = comp::create<ParallelContext>("parallel::openmp", "", prop);
    }
    
    virtual int num_threads() const {
        return local_context_->num_threads();
    }

    virtual bool main_thread() const {
        return local_context_->main_thread();
    }

    virtual void foreach(long long, const ParallelProcessFunc& processFunc, const ProgressUpdateFunc&) const override {
        std::mutex mut;
        std::condition_variable cond;
        bool done = false;

        // Called when the process is completed.
        distributed::worker::on_process_completed([&] {
            std::unique_lock<std::mutex> lock(mut);
            done = true;
            cond.notify_one();
        });

        // Register a function to process a task
        // Note that this function is asynchronious, and called in the different thread.
        distributed::worker::foreach([&](long long start, long long end) {
            local_context_->foreach(end - start, [&](long long index, int threadid) {
                processFunc(start + index, threadid);
            }, [](long long) {
                // Ignore
            });
        });
        
        // Block until completion
        std::unique_lock<std::mutex> lock(mut);
        cond.wait(lock, [&] { return done; });
    }
};

LM_COMP_REG_IMPL(ParallelContext_DistWorker, "parallel::distributed_worker");

LM_NAMESPACE_END(LM_NAMESPACE::parallel)
