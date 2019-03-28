/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/parallel.h>
#include <lm/net.h>
#include <lm/json.h>
#include <lm/logger.h>
#include <lm/progress.h>
#include <zmq.hpp>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::parallel)

class ParallelContext_NetMaster final : public ParallelContext {
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

    virtual void foreach(long long numSamples, const ParallelProcessFunc&) const override {
        LM_INFO("rendering");

        std::mutex mut;
        std::condition_variable cond;
        long long totalProcessed = 0;

        // Called when a task is finished
        net::master::onWorkerTaskFinished([&](long long processed) {
            std::unique_lock<std::mutex> lock(mut);
            totalProcessed += processed;
            LM_INFO("Processed: {}", totalProcessed);
            cond.notify_one();
        });

        // Execute tasks
        const long long WorkSize = 10000;
        const long long Iter = (numSamples + WorkSize - 1) / WorkSize;
        for (long long i = 0; i < Iter; i++) {
            const long long start = i * WorkSize;
            const long long end = std::min((i + 1) * WorkSize, numSamples);
            net::master::processWorkerTask(start, end);
        }

        // Wait for completion
        LM_INFO("Waiting for completion");
        std::unique_lock<std::mutex> lock(mut);
        cond.wait(lock, [&] { return totalProcessed == numSamples; });

        // Notify process has completed
        net::master::notifyProcessCompleted();

        LM_INFO("finish rendering");
    }
};

LM_COMP_REG_IMPL(ParallelContext_NetMaster, "parallel::netmaster");

// ----------------------------------------------------------------------------

class ParallelContext_NetWorker final : public ParallelContext {
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

    virtual void foreach(long long, const ParallelProcessFunc& processFunc) const override {
        LM_INFO("rendering");

        std::mutex mut;
        std::condition_variable cond;
        bool done = false;

        // Called when the process is completed.
        lm::net::worker::onProcessCompleted([&] {
            std::unique_lock<std::mutex> lock(mut);
            done = true;
            cond.notify_one();
        });

        // Register a function to process a task
        // Note that this function is asynchronious, and called in the different thread.
        lm::net::worker::foreach([&](long long start, long long end) {
            {
                progress::ScopedReport progress_(end - start);
                for (long long i = start; i < end; i++) {
                    processFunc(i, 0);
                    progress::update(i - start);
                }
            }
            LM_INFO("");
        });
        
        // Block until completion
        std::unique_lock<std::mutex> lock(mut);
        cond.wait(lock, [&] { return done; });

        LM_INFO("finish rendering");
    }
};

LM_COMP_REG_IMPL(ParallelContext_NetWorker, "parallel::networker");

LM_NAMESPACE_END(LM_NAMESPACE::parallel)
