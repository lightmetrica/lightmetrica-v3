/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <zmq.hpp>
#include <lm/core.h>
#include <lm/debugio.h>
// TODO. Remove the dependency to user.h
#include <lm/user.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::debugio)

// Shared command between server and client
enum class Command {
    handle_message,
    sync_user_context,
    draw
};

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(client)

class DebugioContext_Client final {
private:
    zmq::context_t context_;
    zmq::socket_t socket_;

public:
    static DebugioContext_Client& instance() {
        static DebugioContext_Client instance;
        return instance;
    }

public:
    DebugioContext_Client()
        : context_(1)
        , socket_(context_, ZMQ_REQ)
    {}

public:
    void init(const Json& prop) {
        try {
            if (!context_) {
                // Create a new context when context is invalid.
                // context_ might be closed by shutdown() function.
                zmq::context_t new_context(1);
                context_.swap(new_context);
            }
            socket_.connect(json::value<std::string>(prop, "address"));
        }
        catch (const zmq::error_t& e) {
            LM_THROW_EXCEPTION(Error::None, "ZMQ Error: {}", e.what());
        }
    }

    void shutdown() {
        socket_.close();
        context_.close();
    }

private:
    void call(Command command, const std::string& serialized) {
        zmq::message_t ok;

        // Send command
        zmq::message_t req_command(&command, sizeof(Command));
        socket_.send(req_command, zmq::send_flags::none);
        socket_.recv(ok);

        // Send argument
        zmq::message_t req_args(serialized.data(), serialized.size());
        socket_.send(req_args, zmq::send_flags::none);
        socket_.recv(ok);
    }

public:
    void handle_message(const std::string& message) {
        std::stringstream ss;
        serial::save(ss, message);
        call(Command::handle_message, ss.str());
    }

    void sync_user_context() {
        LM_INFO("Syncing user context");
        std::stringstream ss;
        lm::serialize(ss);
        call(Command::sync_user_context, ss.str());
    }

    void draw(int type, Vec3 color, const std::vector<Vec3>& vs) {
        std::stringstream ss;
        serial::save(ss, type, color, vs);
        call(Command::draw, ss.str());
    }
};

// ------------------------------------------------------------------------------------------------

LM_PUBLIC_API void init(const Json& prop) {
    DebugioContext_Client::instance().init(prop);
}

LM_PUBLIC_API void shutdown() {
    DebugioContext_Client::instance().shutdown();
}

LM_PUBLIC_API void handle_message(const std::string& message) {
    DebugioContext_Client::instance().handle_message(message);
}

LM_PUBLIC_API void sync_user_context() {
    DebugioContext_Client::instance().sync_user_context();
}

LM_PUBLIC_API void draw(int type, Vec3 color, const std::vector<Vec3>& vs) {
    DebugioContext_Client::instance().draw(type, color, vs);
}

LM_NAMESPACE_END(client)

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(server)

class DebugioContext_Server final {
private:
    zmq::context_t context_;
    zmq::socket_t socket_;
    HandleMessageFunc on_handle_message_;
    SyncUserContextFunc on_sync_user_context_;
    DrawFunc on_draw_;

public:
    static DebugioContext_Server& instance() {
        static std::unique_ptr<DebugioContext_Server> instance(new DebugioContext_Server);
        return *instance;
    }

public:
    DebugioContext_Server()
        : context_(1)
        , socket_(context_, ZMQ_REP)
    {}

public:
    void on_handle_message(const HandleMessageFunc& process) {
        on_handle_message_ = process;
    }

    void on_sync_user_context(const SyncUserContextFunc& process) {
        on_sync_user_context_ = process;
    }

    void on_draw(const DrawFunc& process) {
        on_draw_ = process;
    }

public:
    void init(const Json& prop) {
        // Waiting for the connection of the client
        try {
            if (!context_) {
                // Create a new context when context is invalid.
                // context_ might be closed by shutdown() function.
                zmq::context_t new_context(1);
                context_.swap(new_context);
            }
            socket_.bind(json::value<std::string>(prop, "address"));
        }
        catch (const zmq::error_t& e) {
            LM_THROW_EXCEPTION(Error::None, "ZMQ Error: {}", e.what());
        }
    }

    void shutdown() {
        socket_.close();
        context_.close();
        on_handle_message_ = {};
        on_draw_ = {};
    }

    void poll() {
        zmq::pollitem_t item{ socket_, 0, ZMQ_POLLIN, 0 };
        zmq::poll(&item, 1, 0);
        if (item.revents & ZMQ_POLLIN) {
            process_messages();
        }
    }

    void run() {
        while (true) {
            process_messages();
        }
    }

private:
    void process_messages() {
        zmq::message_t ok;

        // Receive command
        zmq::message_t req_command;
        socket_.recv(req_command);
        const auto command = *req_command.data<Command>();
        socket_.send(ok, zmq::send_flags::none);

        // Receive arguments and execute the function
        zmq::message_t req_args;
        socket_.recv(req_args);
        std::stringstream is(std::string(req_args.data<char>(), req_args.size()));
        switch (command) {
            case Command::handle_message: {
                std::string message;
                serial::load(is, message);
                on_handle_message_(message);
                break;
            }
            case Command::sync_user_context: {
                lm::deserialize(is);
                on_sync_user_context_();
                break;
            }
            case Command::draw: {
                int type;
                Vec3 color;
                std::vector<Vec3> vs;
                serial::load(is, type, color, vs);
                on_draw_(type, color, vs);
                break;
            }
        }
        socket_.send(ok, zmq::send_flags::none);
    }
};

// ------------------------------------------------------------------------------------------------

LM_PUBLIC_API void init(const Json& prop) {
    DebugioContext_Server::instance().init(prop);
}

LM_PUBLIC_API void shutdown() {
    DebugioContext_Server::instance().shutdown();
}

LM_PUBLIC_API void poll() {
    DebugioContext_Server::instance().poll();
}

LM_PUBLIC_API void run() {
    DebugioContext_Server::instance().run();
}

LM_PUBLIC_API void on_handle_message(const HandleMessageFunc& process) {
    DebugioContext_Server::instance().on_handle_message(process);
}

LM_PUBLIC_API void on_sync_user_context(const SyncUserContextFunc& process) {
    DebugioContext_Server::instance().on_sync_user_context(process);
}

LM_PUBLIC_API void on_draw(const DrawFunc& process) {
    DebugioContext_Server::instance().on_draw(process);
}

LM_NAMESPACE_END(server)
LM_NAMESPACE_END(LM_NAMESPACE::debugio)
