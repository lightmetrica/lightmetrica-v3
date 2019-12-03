/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/distributed.h>
// TODO. Remove the dependency to user.h
#include <lm/user.h>
#include <lm/parallel.h>
#include <lm/film.h>
#include <lm/progress.h>
#include <zmq.hpp>

#define LM_DIST_MONITOR_SOCKET 1

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::distributed)

using namespace std::chrono_literals;

// Shared commands

// master -> worker, PUB
enum class PubToWorkerCommand {
    workerinfo,
    sync,
    processCompleted,
    gatherFilm,
};

// master -> worker, PUSH
enum class PushToWorkerCommand {
    processWorkerTask,
};

// worker -> master, REQ
enum class ReqToMasterCommand {
    notifyConnection,
};

// worker -> master, PUSH
enum class PushToMasterCommand {
    workerinfo,
    processFunc,
    gatherFilm,
};

// ------------------------------------------------------------------------------------------------

// Worker information
struct WorkerInfo {
    std::string name;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(name);
    }
};

#if LM_DIST_MONITOR_SOCKET
class SocketMonitor final : public zmq::monitor_t {
private:
    std::string name_;

public:
    SocketMonitor(const std::string& name)
        : name_(name)
    {}

public:
    virtual void on_event_connected(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Connected [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_connect_delayed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Delayed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_connect_retried(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Retried [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_listening(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Listening [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_bind_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Bind failed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_accepted(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Accepted [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_accept_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Accept failed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_closed(const zmq_event_t&, const char* addr_) override  {
        LM_INFO("Closed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_close_failed(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Close failed [name='{}', addr='{}']", name_, addr_);
    }
    virtual void on_event_disconnected(const zmq_event_t&, const char* addr_) override {
        LM_INFO("Disconnected [name='{}', addr='{}']", name_, addr_);
    }
};
#endif

// ------------------------------------------------------------------------------------------------

// User-define serialization funciton
using SerializeFunc = std::function<void(std::ostream& os)>;

// Send command with user-defined serialization
template <typename CommandType>
void sendFunc(zmq::socket_t& socket, CommandType command, const SerializeFunc& serialize) {
    std::ostringstream ss;
    lm::serial::save(ss, command);
    serialize(ss);
    const auto str = ss.str();
    socket.send(zmq::message_t(str.data(), str.size()), zmq::send_flags::none);
}

// Send command with arbitrary parameters
template <typename CommandType, typename... Ts>
void send(zmq::socket_t& socket, CommandType command, Ts&&... args) {
    sendFunc(socket, command, [&](std::ostream& os) {
        // This prevents unused argument warning when sizeof..(args) == 0
        LM_UNUSED(os);
        if constexpr (sizeof...(args) > 0) {
            lm::serial::save(os, std::forward<Ts>(args)...);
        }
    });
}

LM_NAMESPACE_END(LM_NAMESPACE::distributed)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::distributed::master)

class MasterContext final {
private:
    int port_;
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> push_socket_;
    std::unique_ptr<zmq::socket_t> pull_socket_;
    std::unique_ptr<zmq::socket_t> pub_socket_;
    std::unique_ptr<zmq::socket_t> rep_socket_;
    #if LM_DIST_MONITOR_SOCKET
    SocketMonitor monitor_rep_socket_;
    #endif
    WorkerTaskFinishedFunc on_worker_task_finished_;
    long long num_issued_tasks_;   // Number of issued tasks
    std::mutex gather_film_mutex_;
    std::condition_variable gather_film_cond_;
    long long gather_film_sync_;
    std::thread event_loop_thread_;
    bool done_ = false;						 // True if the event loop is finished
    bool allow_worker_connection_ = true;	 // True if master allows new connections by workers

public:
    static MasterContext& instance() {
        static MasterContext instance;
        return instance;
    }

public:
    MasterContext()
        : context_(1)
        #if LM_DIST_MONITOR_SOCKET
        , monitor_rep_socket_("rep")
        #endif
    {}

public:
    void init(const Json& prop) {
        port_ = json::value<int>(prop, "port");
        LM_INFO("Listening [port='{}']", port_);

        // Initialize parallel subsystem
        parallel::init("parallel::distributed_master", prop);

        // PUSH and PUB sockets in main thread
        push_socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUSH);
        pub_socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUB);
        push_socket_->bind(fmt::format("tcp://*:{}", port_));
        pub_socket_->bind(fmt::format("tcp://*:{}", port_ + 2));
        
        
        // Thread for event loop
        event_loop_thread_ = std::thread([this]() {
            // PULL and REP sockets in event loop thread
            pull_socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PULL);
            rep_socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_REP);
            pull_socket_->bind(fmt::format("tcp://*:{}", port_ + 1));
            rep_socket_->bind(fmt::format("tcp://*:{}", port_ + 3));
            #if LM_DIST_MONITOR_SOCKET
            monitor_rep_socket_.init(*rep_socket_, "inproc://monitor_rep", ZMQ_EVENT_ALL);
            #endif

            // Event loop
            zmq::pollitem_t items[] = {
                { (void*)*pull_socket_, 0, ZMQ_POLLIN, 0 },
                { (void*)*rep_socket_, 0, ZMQ_POLLIN, 0 },
            };
            while (!done_) {
                #if LM_DIST_MONITOR_SOCKET
                // Monitor socket
                monitor_rep_socket_.check_event();
                #endif

                // Handle events
                zmq::poll(items, 2, 0);

                // PULL socket
                if (items[0].revents & ZMQ_POLLIN) {
                    // Receive message
                    zmq::message_t mes;
                    pull_socket_->recv(mes);
                    std::istringstream is(std::string(mes.data<char>(), mes.size()));

                    // Extract command
                    PushToMasterCommand command;
                    lm::serial::load(is, command);

                    // Process command
                    if (command == PushToMasterCommand::workerinfo) {
                        WorkerInfo info;
                        lm::serial::load(is, info);
                        LM_INFO("Worker [name='{}']", info.name);
                    }
                    else if (command == PushToMasterCommand::processFunc) {
                        long long processed;
                        lm::serial::load(is, processed);
                        on_worker_task_finished_(processed);
                    }
                    else if (command == PushToMasterCommand::gatherFilm) {
                        long long numProcessedTasks;
                        std::string filmloc;
                        lm::serial::load(is, numProcessedTasks, filmloc);
                        Component::Ptr<Film> workerFilm;
                        lm::serial::load(is, workerFilm);
                        auto* film = lm::comp::get<Film>(filmloc);
                        film->accum(workerFilm.get());
                        std::unique_lock<std::mutex> lock(gather_film_mutex_);
                        gather_film_sync_ += numProcessedTasks;
                        gather_film_cond_.notify_one();
                    }
                }

                // REP socket
                if ((items[1].revents & ZMQ_POLLIN) && allow_worker_connection_) {
                    zmq::message_t mes;
                    rep_socket_->recv(mes);
                    std::istringstream is(std::string(mes.data<char>(), mes.size()));
                    ReqToMasterCommand command;
                    lm::serial::load(is, command);
                    if (command == ReqToMasterCommand::notifyConnection) {
                        WorkerInfo info;
                        lm::serial::load(is, info);
                        LM_INFO("Connected worker [name='{}']", info.name);
                        zmq::message_t ok;
                        rep_socket_->send(ok, zmq::send_flags::none);
                    }
                }
            }
        });
    }

    void shutdown() {
        done_ = true;
        event_loop_thread_.join();
    }

public:
    void printWorkerInfo() {
        send(*pub_socket_, PubToWorkerCommand::workerinfo);
    }

    void allowWorkerConnection(bool allow) {
        allow_worker_connection_ = allow;
    }

    void sync() {
        // Synchronize internal state and dispatch rendering in worker process
        sendFunc(*pub_socket_, PubToWorkerCommand::sync, [&](std::ostream& os) {
            lm::serialize(os);
        });
    }

    void onWorkerTaskFinished(const WorkerTaskFinishedFunc& func) {
        on_worker_task_finished_ = func;
    }

    void processWorkerTask(long long start, long long end) {
        if (start == 0) {
            // Reset the issued task
            num_issued_tasks_ = 0;
        }
        send(*push_socket_, PushToWorkerCommand::processWorkerTask, start, end);
        num_issued_tasks_++;
    }

    void notifyProcessCompleted()  {
        send(*pub_socket_, PubToWorkerCommand::processCompleted);
    }

    void gatherFilm(const std::string& filmloc) {
        // Initialize film
        gather_film_sync_ = 0;
        lm::comp::get<Film>(filmloc)->clear();

        // Send command
        send(*pub_socket_, PubToWorkerCommand::gatherFilm, filmloc);

        // Synchronize
        progress::ScopedReport progress_(num_issued_tasks_);
        std::unique_lock<std::mutex> lock(gather_film_mutex_);
        gather_film_cond_.wait(lock, [&] {
            progress::update(gather_film_sync_);
            return gather_film_sync_ == num_issued_tasks_;
        });
    }
};

LM_PUBLIC_API void init(const Json& prop) {
    MasterContext::instance().init(prop);
}

LM_PUBLIC_API void shutdown() {
    MasterContext::instance().shutdown();
}

LM_PUBLIC_API void print_worker_info() {
    MasterContext::instance().printWorkerInfo();
}

LM_PUBLIC_API void allow_worker_connection(bool allow) {
    MasterContext::instance().allowWorkerConnection(allow);
}

LM_PUBLIC_API void sync() {
    MasterContext::instance().sync();
}

LM_PUBLIC_API void on_worker_task_finished(const WorkerTaskFinishedFunc& func) {
    MasterContext::instance().onWorkerTaskFinished(func);
}

LM_PUBLIC_API void process_worker_task(long long start, long long end) {
    MasterContext::instance().processWorkerTask(start, end);
}

LM_PUBLIC_API void notify_process_completed() {
    MasterContext::instance().notifyProcessCompleted();
}

LM_PUBLIC_API void gather_film(const std::string& filmloc) {
    MasterContext::instance().gatherFilm(filmloc);
}

LM_NAMESPACE_END(LM_NAMESPACE::distributed::master)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::distributed::worker)

class WorkerContext final {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> pull_socket_;
    std::unique_ptr<zmq::socket_t> push_socket_;
    std::unique_ptr<zmq::socket_t> sub_socket_;
    std::unique_ptr<zmq::socket_t> req_socket_;
    std::string name_;
    #if LM_DIST_MONITOR_SOCKET
    SocketMonitor monitor_req_socket_;
    #endif
    WorkerProcessFunc process_func_;
    ProcessCompletedFunc process_completed_func_;
    std::thread render_thread_;
    long long num_processed_tasks_;  // Number of procesed tasks

public:
    static WorkerContext& instance() {
        static WorkerContext instance;
        return instance;
    }

public:
    WorkerContext()
        : context_(1)
        #if LM_DIST_MONITOR_SOCKET
        , monitor_req_socket_("req")
        #endif
    {}

public:
    void init(const Json& prop) {
        name_ = json::value<std::string>(prop, "name");
        const auto address = json::value<std::string>(prop, "address");
        const auto port = json::value<int>(prop, "port");

        // First try to connect only with REQ socket.
        // Once a connection is established, connect with other sockets.
        req_socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_REQ);
        req_socket_->connect(fmt::format("tcp://{}:{}", address, port+3));
        #if LM_DIST_MONITOR_SOCKET
        // Register monitors
        monitor_req_socket_.init(*req_socket_, "inproc://monitor_req", ZMQ_EVENT_ALL);
        #endif

        // Synchronize
        // This is necessary to avoid missing initial messages of PUB-SUB connection.
        // cf. http://zguide.zeromq.org/page:all#Node-Coordination
        while (true) {
            // Send worker information
            send(*req_socket_, ReqToMasterCommand::notifyConnection, WorkerInfo{ name_ });
            zmq::message_t mes;
            if (req_socket_->recv(mes)) {
                break;
            }
        }

        // Create sockets
        pull_socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PULL);
        push_socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUSH);
        sub_socket_ = std::make_unique<zmq::socket_t>(context_, ZMQ_SUB);

        // Connect
        LM_INFO("Connecting [addr='{}', port='{}']", address, port);
        pull_socket_->connect(fmt::format("tcp://{}:{}", address, port));
        push_socket_->connect(fmt::format("tcp://{}:{}", address, port+1));
        sub_socket_->connect(fmt::format("tcp://{}:{}", address, port+2));
        sub_socket_->setsockopt(ZMQ_SUBSCRIBE, "", 0);

        // Initialize parallel subsystem
        parallel::init("parallel::distributed_worker", prop);
    }

    void shutdown() {}

    void foreach(const WorkerProcessFunc& process) {
        process_func_ = process;
        num_processed_tasks_ = 0;
    }

    void onProcessCompleted(const ProcessCompletedFunc& func) {
        process_completed_func_ = func;
    }

    void run() {
        zmq::pollitem_t items[] = {
            { (void*)*pull_socket_, 0, ZMQ_POLLIN, 0 },
            { (void*)*sub_socket_, 0, ZMQ_POLLIN, 0 },
        };
        while (true) {
            #if LM_DIST_MONITOR_SOCKET
            // Monitor socket
            monitor_req_socket_.check_event();
            #endif

            // Handle events
            zmq::poll(items, 2, 0);

            // PULL socket
            if (items[0].revents & ZMQ_POLLIN) [&]{
                // Process function is not ready, wait for render() function being called
                if (!process_func_) {
                    return;
                }

                // Receive message
                zmq::message_t mes;
                pull_socket_->recv(mes);
                std::istringstream is(std::string(mes.data<char>(), mes.size()));

                // Extract command
                PushToWorkerCommand command;
                lm::serial::load(is, command);

                if (command == PushToWorkerCommand::processWorkerTask) {
                    // Arguments
                    long long start;
                    long long end;
                    lm::serial::load(is, start, end);
                
                    // Process a task
                    num_processed_tasks_++;
                    process_func_(start, end);

                    // Notify completion
                    // Send processed number of samples
                    send(*push_socket_, PushToMasterCommand::processFunc, end - start);
                }
            }();

            // SUB socket
            if (items[1].revents & ZMQ_POLLIN) {
                // Receive message
                zmq::message_t mes;
                sub_socket_->recv(mes);
                std::istringstream is(std::string(mes.data<char>(), mes.size()));

                // Extract command
                PubToWorkerCommand command;
                lm::serial::load(is, command);

                // Process commands
                if (command == PubToWorkerCommand::workerinfo) {
                    send(*push_socket_, PushToMasterCommand::workerinfo, WorkerInfo{ name_ });
                }
                else if (command == PubToWorkerCommand::sync) {
                    lm::deserialize(is);

                    // Dispatch renderer in the different thread
                    // to prevent early return of the Renderer::render() function.
                    render_thread_ = std::thread([&] {
                        lm::render();
                    });
                }
                else if (command == PubToWorkerCommand::processCompleted) {;
                    process_completed_func_();
                    render_thread_.join();
                    process_func_ = {};
                    process_completed_func_ = {};
                }
                else if (command == PubToWorkerCommand::gatherFilm) {
                    std::string filmloc;
                    lm::serial::load(is, filmloc);
                    sendFunc(*push_socket_, PushToMasterCommand::gatherFilm, [&](std::ostream& os) {
                        lm::serial::save(os, num_processed_tasks_, filmloc);
                        lm::serial::save_owned(os, lm::comp::get<Film>(filmloc));
                    });
                }
            }
        }
    }
};

LM_PUBLIC_API void init(const Json& prop) {
    WorkerContext::instance().init(prop);
}

LM_PUBLIC_API void shutdown() {
    WorkerContext::instance().shutdown();
}

LM_PUBLIC_API void run() {
    WorkerContext::instance().run();
}

LM_PUBLIC_API void on_process_completed(const ProcessCompletedFunc& func) {
    WorkerContext::instance().onProcessCompleted(func);
}

LM_PUBLIC_API void foreach(const WorkerProcessFunc& process) {
    WorkerContext::instance().foreach(process);
}

LM_NAMESPACE_END(LM_NAMESPACE::distributed::worker)
