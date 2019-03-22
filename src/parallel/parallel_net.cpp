/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/parallel.h>
#include <lm/net.h>
#include <lm/json.h>
#include <lm/logger.h>
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

    virtual void foreach(long long, const ParallelProcessFunc&) const override {
        //const long long WorkSize = 100000;
        //const long long Iter = (numSamples + WorkSize - 1) / WorkSize;
        //for (long long i = 0; i < Iter; i++) {
        //    const long long work = std::min(WorkSize, numSamples - i * WorkSize);
        //    zmq::message_t mes(&work, sizeof(long long));
        //    socket_->send(mes);
        //}
        
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

    virtual void foreach(long long, const ParallelProcessFunc&) const override {
        
    }
};

LM_COMP_REG_IMPL(ParallelContext_NetWorker, "parallel::networker");

LM_NAMESPACE_END(LM_NAMESPACE::parallel)
