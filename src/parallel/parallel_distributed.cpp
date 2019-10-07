/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/parallel.h>
#include <lm/dist.h>
#include <lm/json.h>
#include <zmq.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel)

class ParallelContext_DistMaster final : public ParallelContext {
public:
    virtual bool construct(const Json&) override {
        return true;
    }

    virtual int numThreads() const {
        return 0;
    }

    virtual bool mainThread() const {
        return true;
    }

    virtual void foreach(long long numSamples, const ParallelProcessFunc&, const ProgressUpdateFunc& progressUpdateFunc) const override {
        std::mutex mut;
        std::condition_variable cond;
        long long totalProcessed = 0;

        // Called when a task is finished
        distributed::master::onWorkerTaskFinished([&](long long processed) {
            std::unique_lock<std::mutex> lock(mut);
            totalProcessed += processed;
            cond.notify_one();
        });

        // Execute tasks
        const long long WorkSize = 10000;
        const long long Iter = (numSamples + WorkSize - 1) / WorkSize;
        for (long long i = 0; i < Iter; i++) {
            const long long start = i * WorkSize;
            const long long end = std::min((i + 1) * WorkSize, numSamples);
            distributed::master::processWorkerTask(start, end);
        }

        // Wait for completion
        std::unique_lock<std::mutex> lock(mut);
        cond.wait(lock, [&] {
            progressUpdateFunc(totalProcessed);
            return totalProcessed == numSamples;
        });

        // Notify process has completed
        distributed::master::notifyProcessCompleted();
    }
};

LM_COMP_REG_IMPL(ParallelContext_DistMaster, "parallel::distributed_master");

// ----------------------------------------------------------------------------

class ParallelContext_DistWorker final : public ParallelContext {
public:
    Ptr<ParallelContext> localContext_;

public:
    virtual bool construct(const Json& prop) override {
        localContext_ = comp::create<ParallelContext>("parallel::openmp", "", prop);
        return true;
    }
    
    virtual int numThreads() const {
        return localContext_->numThreads();
    }

    virtual bool mainThread() const {
        return localContext_->mainThread();
    }

    virtual void foreach(long long, const ParallelProcessFunc& processFunc, const ProgressUpdateFunc&) const override {
        std::mutex mut;
        std::condition_variable cond;
        bool done = false;

        // Called when the process is completed.
        distributed::worker::onProcessCompleted([&] {
            std::unique_lock<std::mutex> lock(mut);
            done = true;
            cond.notify_one();
        });

        // Register a function to process a task
        // Note that this function is asynchronious, and called in the different thread.
        distributed::worker::foreach([&](long long start, long long end) {
            localContext_->foreach(end - start, [&](long long index, int threadId) {
                processFunc(start + index, threadId);
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
