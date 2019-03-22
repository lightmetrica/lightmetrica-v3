/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/net.h>
#include <lm/logger.h>
#include <lm/json.h>
#include <lm/user.h>
#include <lm/parallel.h>
#include <lm/serial.h>
#include <zmq.hpp>

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::net)

using namespace std::chrono_literals;

// Shared command distributed to workers
enum class WorkerCommand {
    workerinfo,     // Query worker information
    sync,           // Synchronize internal data
    render,         // Dispatch renderer
};

// Shared command for return value from workers
enum class ReturnCommand {
    workerinfo,
};

// Worker information
struct WorkerInfo {
    std::string name;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(name);
    }
};

class SocketMonitor final : public zmq::monitor_t {
public:
    virtual void on_event_connected(const zmq_event_t&, const char* addr_) override {
        LM_INFO("connected [addr='{}']", addr_);
    }
    virtual void on_event_connect_delayed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("connect_delayed [addr='{}']", addr_);
    }
    virtual void on_event_connect_retried(const zmq_event_t&, const char* addr_) override {
        LM_INFO("connect_retried [addr='{}']", addr_);
    }
    virtual void on_event_listening(const zmq_event_t&, const char* addr_) override {
        LM_INFO("listening [addr='{}']", addr_);
    }
    virtual void on_event_bind_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("failed [addr='{}']", addr_);
    }
    virtual void on_event_accepted(const zmq_event_t&, const char* addr_) override {
        LM_INFO("accepted [addr='{}']", addr_);
    }
    virtual void on_event_accept_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("accept_failed [addr='{}']", addr_);
    }
    virtual void on_event_closed(const zmq_event_t&, const char* addr_) override  {
        LM_INFO("closed [addr='{}']", addr_);
    }
    virtual void on_event_close_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("failed [addr='{}']", addr_);
    }
    virtual void on_event_disconnected(const zmq_event_t&, const char* addr_) override {
        LM_INFO("disconnected [addr='{}']", addr_);
    }
    virtual void on_event_unknown(const zmq_event_t&, const char* addr_) override {
        LM_INFO("unknown [addr='{}']", addr_);
    }
};

LM_NAMESPACE_END(LM_NAMESPACE::net)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::net::master)

class NetMasterContext_ final : public NetMasterContext {
private:
    zmq::context_t context_;
    zmq::socket_t pushSocket_;
    zmq::socket_t pullSocket_;
    zmq::socket_t pubSocket_;
    SocketMonitor monitor_;
    int numWorkers_ = 0;

public:
    NetMasterContext_()
        : context_(1)
        , pushSocket_(context_, ZMQ_PUSH)
        , pullSocket_(context_, ZMQ_PULL)
        , pubSocket_(context_, ZMQ_PUB)
    {}

public:
    virtual bool construct(const Json&) override {
        // Initialize parallel subsystem
        parallel::init("parallel::netmaster", {});
        // Register monitor for PUB socket
        monitor_.init(pubSocket_, "inproc://monitor", ZMQ_EVENT_ALL);
        return true;
    }

    virtual void monitor() {
        std::thread([&]() {
            while (true) {
                monitor_.check_event();
            }
        }).detach();
    }

    virtual void addWorker(const std::string& address, int port) override {
        LM_INFO("Connecting [addr='{}', port='{}-{}']", address, port, port+2);
        pushSocket_.connect(fmt::format("tcp://{}:{}", address, port));
        pullSocket_.connect(fmt::format("tcp://{}:{}", address, port + 1));
        pubSocket_.connect(fmt::format("tcp://{}:{}", address, port + 2));
        numWorkers_++;
    }

    virtual void printWorkerInfo() override {
        // Send a command
        {
            std::ostringstream os;
            lm::serial::save(os, WorkerCommand::workerinfo);
            const auto str = os.str();
            pubSocket_.send(zmq::message_t(str.data(), str.size()));
        }

        // Wait for reply
#if 1
        int count = 0;
        while (count < numWorkers_) {
            zmq::message_t mes;
            pullSocket_.recv(&mes);
            std::istringstream is(std::string(mes.data<char>(), mes.size()));
            WorkerInfo info;
            lm::serial::load(is, info);
            LM_INFO("Worker [name='{}']", info.name);
            count++;
        }
#else
        zmq::pollitem_t item{ pullSocket_, 0, ZMQ_POLLIN, 0 };
        int count = 0;
        while (count == numWorkers_) {
            zmq::poll(&item, 1, 0);
            if (item.revents & ZMQ_POLLIN) {
                zmq::message_t mes;
                pullSocket_.recv(&mes);          
                count++;
            }
        }
#endif
    }

    virtual void render() override {
#if 0
        // 1. Notify the information of the workers, create a list of workers.
        {
            std::stringstream ss;
            lm::serial::save(ss, SubPubCommand::workerinfo);
            const auto str = ss.str();
            zmq::message_t mes(str.data(), str.size());
            pubSocket_.send(mes);
        }

        // 1-2. Wait for reply.
        

        // 1. Synchronize internal state
        {
            std::stringstream ss;
            lm::serial::save(ss, SubPubCommand::sync);
            lm::serialize(ss);
            const auto str = ss.str();
            zmq::message_t mes(str.data(), str.size());
            pubSocket_.send(mes);
        }

        // 2. Execute renderer of workers
        {
            std::stringstream ss;
            lm::serial::save(ss, SubPubCommand::render);
            const auto str = ss.str();
            zmq::message_t mes(str.data(), str.size());
            pubSocket_.send(mes);
        }

        // 3. Wait until workers are ready
        
        

        // 3. Execute renderer of master
        lm::render();

        // 4. Wait for 

        // 2. Execute rendering both in master and worker
        call(Command::render, "");
        lm::render();
#endif
    }
};

LM_COMP_REG_IMPL(NetMasterContext_, "net::master::default");

// ----------------------------------------------------------------------------

using Instance = comp::detail::ContextInstance<NetMasterContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void monitor() {
    Instance::get().monitor();
}

LM_PUBLIC_API void addWorker(const std::string& address, int port) {
    Instance::get().addWorker(address, port);
}

LM_PUBLIC_API void printWorkerInfo() {
    Instance::get().printWorkerInfo();
}

LM_PUBLIC_API void render() {
    Instance::get().render();
}

LM_NAMESPACE_END(LM_NAMESPACE::net::master)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::net::worker)

class NetWorkerContext_ final : public NetWorkerContext {
private:
    zmq::context_t context_;
    zmq::socket_t pullSocket_;
    zmq::socket_t pushSocket_;
    zmq::socket_t subSocket_;
    std::string name_;
    SocketMonitor monitor_;

public:
    NetWorkerContext_()
        : context_(1)
        , pullSocket_(context_, ZMQ_PULL)
        , pushSocket_(context_, ZMQ_PUSH)
        , subSocket_(context_, ZMQ_SUB)
    {}

public:
    virtual bool construct(const Json& prop) override {
        name_ = json::value<std::string>(prop, "name");

        const int port = json::value<int>(prop, "port");
        LM_INFO("Listening [port='{}-{}']", port, port+2);
        pullSocket_.bind(fmt::format("tcp://*:{}", port));
        pushSocket_.bind(fmt::format("tcp://*:{}", port + 1));
        subSocket_.bind(fmt::format("tcp://*:{}", port + 2));
        subSocket_.setsockopt(ZMQ_SUBSCRIBE, "");

        // Initialize parallel subsystem
        parallel::init("parallel::networker", {});

        // Register monitor for SUB socket
        monitor_.init(subSocket_, "inproc://monitor", ZMQ_EVENT_ALL);

        return true;
    }

    virtual void run() override {
        zmq::pollitem_t items[] = {
            { (void*)pullSocket_, 0, ZMQ_POLLIN, 0 },
            { (void*)subSocket_, 0, ZMQ_POLLIN, 0 }
        };
        while (true) {
            // Monitor socket
            monitor_.check_event();

            // Handle events
            zmq::poll(items, 2, 0);
            // PULL socket
            if (items[0].revents & ZMQ_POLLIN) {
                
            }
            // SUB socket
            if (items[1].revents & ZMQ_POLLIN) {
                LM_INFO("SUB");

                // Receive message
                zmq::message_t mes;
                subSocket_.recv(&mes);
                std::istringstream is(std::string(mes.data<char>(), mes.size()));

                // Extract command
                WorkerCommand command;
                lm::serial::load(is, command);

                // Process commands
                if (command == WorkerCommand::workerinfo) {
                    LM_INFO("workerinfo");

                    // Send worker information
                    WorkerInfo info{ name_ };
                    std::ostringstream os;
                    lm::serial::save(os, ReturnCommand::workerinfo);
                    lm::serial::save(os, info);
                    const auto str = os.str();
                    pushSocket_.send(zmq::message_t(str.data(), str.size()));
                }
            }
        }
    }
};

LM_COMP_REG_IMPL(NetWorkerContext_, "net::worker::default");

// ----------------------------------------------------------------------------

using Instance = comp::detail::ContextInstance<NetWorkerContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void run() {
    Instance::get().run();
}

LM_NAMESPACE_END(LM_NAMESPACE::net::worker)
