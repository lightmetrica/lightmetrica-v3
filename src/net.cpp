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

#define LM_NET_MONITOR_SOCKET 1

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::net)

using namespace std::chrono_literals;

// Shared command distributed to workers
enum class WorkerCommand {
    workerinfo,
    sync,
    processCompleted,
};

// Shared command for return value from workers
enum class WorkerResultCommand {
    workerinfo,
    processFunc,
};

// Worker information
struct WorkerInfo {
    std::string name;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(name);
    }
};

#if LM_NET_MONITOR_SOCKET
class SocketMonitor final : public zmq::monitor_t {
private:
    std::string name_;

public:
    SocketMonitor(const std::string& name)
        : name_(name)
    {}

public:
    virtual void on_event_connected(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_connected [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_connect_delayed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_connect_delayed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_connect_retried(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_connect_retried [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_listening(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_listening [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_bind_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_bind_failed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_accepted(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_accepted [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_accept_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_accept_failed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_closed(const zmq_event_t&, const char* addr_) override  {
        LM_INFO("on_event_closed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_close_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_close_failed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_disconnected(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_disconnected [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_unknown(const zmq_event_t&, const char* addr_) override {
        LM_INFO("on_event_unknown [name='{}', addr='{}']", name_, addr_);
    }
};
#endif

LM_NAMESPACE_END(LM_NAMESPACE::net)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::net::master)

class NetMasterContext_ final : public NetMasterContext {
private:
    int port_;
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> pushSocket_;
    std::unique_ptr<zmq::socket_t> pullSocket_;
    std::unique_ptr<zmq::socket_t> pubSocket_;
    std::unique_ptr<zmq::socket_t> repSocket_;
    #if LM_NET_MONITOR_SOCKET
    SocketMonitor monitor_repSocket_;
    #endif
    WorkerTaskFinishedFunc onWorkerTaskFinished_;

public:
    NetMasterContext_()
        : context_(1)
        #if LM_NET_MONITOR_SOCKET
        , monitor_repSocket_("rep")
        #endif
    {}

public:
    virtual bool construct(const Json& prop) override {
        port_ = json::value<int>(prop, "port");
        LM_INFO("Listening [port='{}']", port_);
        
        // --------------------------------------------------------------------

        // Initialize parallel subsystem
        parallel::init("parallel::netmaster", {});

        // --------------------------------------------------------------------

        // PUSH and PUB sockets in main thread
        pushSocket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUSH);
        pubSocket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUB);
        pushSocket_->bind(fmt::format("tcp://*:{}", port_));
        pubSocket_->bind(fmt::format("tcp://*:{}", port_ + 2));
        
        // --------------------------------------------------------------------

        // Thread for event loop
        std::thread([this]() {
            // PULL and REP sockets in event loop thread
            pullSocket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PULL);
            repSocket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_REP);
            pullSocket_->bind(fmt::format("tcp://*:{}", port_ + 1));
            repSocket_->bind(fmt::format("tcp://*:{}", port_ + 3));
            #if LM_NET_MONITOR_SOCKET
            monitor_repSocket_.init(*repSocket_, "inproc://monitor_rep", ZMQ_EVENT_ALL);
            #endif

            // ----------------------------------------------------------------

            // Event loop
            zmq::pollitem_t items[] = {
                { (void*)*pullSocket_, 0, ZMQ_POLLIN, 0 },
                { (void*)*repSocket_, 0, ZMQ_POLLIN, 0 },
            };
            while (true) {
                #if LM_NET_MONITOR_SOCKET
                // Monitor socket
                monitor_repSocket_.check_event();
                #endif

                // Handle events
                zmq::poll(items, 2, 0);

                // PULL socket
                if (items[0].revents & ZMQ_POLLIN) {
                    LM_INFO("PULL");

                    // Receive message
                    zmq::message_t mes;
                    pullSocket_->recv(&mes);
                    std::istringstream is(std::string(mes.data<char>(), mes.size()));

                    // Extract command
                    WorkerResultCommand command;
                    lm::serial::load(is, command);

                    // Process command
                    if (command == WorkerResultCommand::workerinfo) {
                        WorkerInfo info;
                        lm::serial::load(is, info);
                        LM_INFO("Worker [name='{}']", info.name);
                    }
                    else if (command == WorkerResultCommand::processFunc) {
                        long long processed;
                        lm::serial::load(is, processed);
                        onWorkerTaskFinished_(processed);
                    }
                }

                // REP socket
                if (items[1].revents & ZMQ_POLLIN) {
                    LM_INFO("REP");
                    zmq::message_t mes;
                    repSocket_->recv(&mes);
                    std::istringstream is(std::string(mes.data<char>(), mes.size()));
                    WorkerInfo info;
                    lm::serial::load(is, info);
                    LM_INFO("Connected worker [name='{}']", info.name);
                    zmq::message_t ok;
                    repSocket_->send(ok);
                }
            }
        }).detach();
        
        return true;
    }

public:
    virtual void printWorkerInfo() override {
        std::ostringstream os;
        lm::serial::save(os, WorkerCommand::workerinfo);
        const auto str = os.str();
        pubSocket_->send(zmq::message_t(str.data(), str.size()));
    }

    virtual void render() override {
        // Synchronize internal state and dispatch rendering in worker process
        std::stringstream ss;
        lm::serial::save(ss, WorkerCommand::sync);
        lm::serialize(ss);
        const auto str = ss.str();
        pubSocket_->send(zmq::message_t(str.data(), str.size()));

        // Execute renderer in master process
        lm::render();
    }

    virtual void onWorkerTaskFinished(const WorkerTaskFinishedFunc& func) override {
        onWorkerTaskFinished_ = func;
    }

    virtual void processWorkerTask(long long start, long long end) override {
        LM_INFO("processWorkerTask");
        std::stringstream ss;
        lm::serial::save(ss, start);
        lm::serial::save(ss, end);
        const auto str = ss.str();
        pushSocket_->send(zmq::message_t(str.data(), str.size()));
    }

    virtual void notifyProcessCompleted() {
        LM_INFO("notifyProcessCompleted");
        std::stringstream ss;
        lm::serial::save(ss, WorkerCommand::processCompleted);
        const auto str = ss.str();
        pubSocket_->send(zmq::message_t(str.data(), str.size()));
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

LM_PUBLIC_API void printWorkerInfo() {
    Instance::get().printWorkerInfo();
}

LM_PUBLIC_API void render() {
    Instance::get().render();
}

LM_PUBLIC_API void onWorkerTaskFinished(const WorkerTaskFinishedFunc& func) {
    Instance::get().onWorkerTaskFinished(func);
}

LM_PUBLIC_API void processWorkerTask(long long start, long long end) {
    Instance::get().processWorkerTask(start, end);
}

LM_PUBLIC_API void notifyProcessCompleted() {
    Instance::get().notifyProcessCompleted();
}

LM_NAMESPACE_END(LM_NAMESPACE::net::master)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::net::worker)

//std::atomic<bool> interrupted_ = false;

class NetWorkerContext_ final : public NetWorkerContext {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> pullSocket_;
    std::unique_ptr<zmq::socket_t> pushSocket_;
    std::unique_ptr<zmq::socket_t> subSocket_;
    std::unique_ptr<zmq::socket_t> reqSocket_;
    std::string name_;
    #if LM_NET_MONITOR_SOCKET
    SocketMonitor monitor_reqSocket_;
    #endif
    NetWorkerProcessFunc processFunc_;
    ProcessCompletedFunc processCompletedFunc_;
    std::thread renderThread_;

public:
    NetWorkerContext_()
        : context_(1)
        #if LM_NET_MONITOR_SOCKET
        , monitor_reqSocket_("req")
        #endif
    {}

public:
    virtual bool construct(const Json& prop) override {
        name_ = json::value<std::string>(prop, "name");
        const auto address = json::value<std::string>(prop, "address");
        const auto port = json::value<int>(prop, "port");

        // Create thread
        pullSocket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PULL);
        pushSocket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUSH);
        subSocket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_SUB);
        reqSocket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_REQ);

        // Connect
        LM_INFO("Connecting [addr='{}', port='{}']", address, port);
        pullSocket_->connect(fmt::format("tcp://{}:{}", address, port));
        pushSocket_->connect(fmt::format("tcp://{}:{}", address, port+1));
        subSocket_->connect(fmt::format("tcp://{}:{}", address, port+2));
        subSocket_->setsockopt(ZMQ_SUBSCRIBE, "", 0);
        reqSocket_->connect(fmt::format("tcp://{}:{}", address, port+3));

        // Initialize parallel subsystem
        parallel::init("parallel::networker", {});

        #if LM_NET_MONITOR_SOCKET
        // Register monitors
        monitor_reqSocket_.init(*reqSocket_, "inproc://monitor_req", ZMQ_EVENT_ALL);
        #endif

        // Synchronize
        // This is necessary to avoid missing initial messages of PUB-SUB connection.
        // cf. http://zguide.zeromq.org/page:all#Node-Coordination
        {
            // Send worker information
            WorkerInfo info{ name_ };
            std::ostringstream os;
            lm::serial::save(os, info);
            const auto str = os.str();
            reqSocket_->send(zmq::message_t(str.data(), str.size()));
            zmq::message_t mes;
            reqSocket_->recv(&mes);
            LM_INFO("Connected");
        }

        return true;
    }

    virtual void foreach(const NetWorkerProcessFunc& process) override {
        processFunc_ = process;
    }

    virtual void onProcessCompleted(const ProcessCompletedFunc& func) override {
        processCompletedFunc_ = func;
    }

    virtual void run() override {
        zmq::pollitem_t items[] = {
            { (void*)*pullSocket_, 0, ZMQ_POLLIN, 0 },
            { (void*)*subSocket_, 0, ZMQ_POLLIN, 0 },
        };
        while (true) {
            #if LM_NET_MONITOR_SOCKET
            // Monitor socket
            monitor_reqSocket_.check_event();
            #endif

            // Handle events
            zmq::poll(items, 2, 0);

            // PULL socket
            if (items[0].revents & ZMQ_POLLIN) [&]{
                // Process function is not ready, wait for render() function being called
                if (!processFunc_) {
                    return;
                }

                // Receive message
                zmq::message_t mes;
                pullSocket_->recv(&mes);

                // Arguments
                std::istringstream is(std::string(mes.data<char>(), mes.size()));
                long long start;
                lm::serial::load(is, start);
                long long end;
                lm::serial::load(is, end);
                
                // Process a task
                processFunc_(start, end);

                // Notify completion
                // Send processed number of samples
                {
                    std::ostringstream os;
                    lm::serial::save(os, WorkerResultCommand::processFunc);
                    lm::serial::save(os, end - start);
                    const auto str = os.str();
                    pushSocket_->send(zmq::message_t(str.data(), str.size()));
                }
            }();

            // SUB socket
            if (items[1].revents & ZMQ_POLLIN) {
                LM_INFO("SUB");

                // Receive message
                zmq::message_t mes;
                subSocket_->recv(&mes);
                std::istringstream is(std::string(mes.data<char>(), mes.size()));

                // Extract command
                WorkerCommand command;
                lm::serial::load(is, command);

                // Process commands
                if (command == WorkerCommand::workerinfo) {
                    LM_INFO("workerinfo");
                    WorkerInfo info{ name_ };
                    std::ostringstream os;
                    lm::serial::save(os, WorkerResultCommand::workerinfo);
                    lm::serial::save(os, info);
                    const auto str = os.str();
                    pushSocket_->send(zmq::message_t(str.data(), str.size()));
                }
                else if (command == WorkerCommand::sync) {
                    LM_INFO("sync");
                    lm::deserialize(is);

                    // Dispatch renderer in the different thread
                    // to prevent early return of the Renderer::render() function.
                    renderThread_ = std::thread([&] {
                        lm::render();
                    });
                }
                else if (command == WorkerCommand::processCompleted) {
                    LM_INFO("processCompleted");
                    processCompletedFunc_();
                    renderThread_.join();
                    processFunc_ = {};
                    processCompletedFunc_ = {};
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

LM_PUBLIC_API void onProcessCompleted(const ProcessCompletedFunc& func) {
    Instance::get().onProcessCompleted(func);
}

LM_PUBLIC_API void foreach(const NetWorkerProcessFunc& process) {
    Instance::get().foreach(process);
}

LM_NAMESPACE_END(LM_NAMESPACE::net::worker)
