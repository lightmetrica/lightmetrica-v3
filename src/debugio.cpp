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

// https://stackoverflow.com/questions/7781898/get-an-istream-from-a-char
struct membuf : std::streambuf {
    membuf(char const* base, size_t size) {
        char* p(const_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};
struct imemstream : virtual membuf, std::istream {
    imemstream(char const* base, size_t size)
        : membuf(base, size)
        , std::istream(static_cast<std::streambuf*>(this)) {
    }
};

// ------------------------------------------------------------------------------------------------

// Shared command between server and client
enum class Command {
    handleMessage,
    syncUserContext,
    draw
};

// ------------------------------------------------------------------------------------------------

class DebugioContext_Client final : public DebugioContext {
private:
    zmq::context_t context_;
    zmq::socket_t socket_;

public:
    DebugioContext_Client()
        : context_(1)
        , socket_(context_, ZMQ_REQ)
    {}

public:
    virtual bool construct(const Json& prop) override {
        try {
            socket_.connect(prop["address"]);
        }
        catch (const zmq::error_t& e) {
            LM_ERROR("ZMQ Error: {}", e.what());
            return false;
        }
        return true;
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
    virtual void handleMessage(const std::string& message) override {
        std::stringstream ss;
        serial::save(ss, message);
        call(Command::handleMessage, ss.str());
    }

    virtual void syncUserContext() override {
        LM_INFO("Syncing user context");
        std::stringstream ss;
        lm::serialize(ss);
        call(Command::syncUserContext, ss.str());
    }

    virtual void draw(int type, Vec3 color, const std::vector<Vec3>& vs) override {
        std::stringstream ss;
        serial::save(ss, type, color, vs);
        call(Command::draw, ss.str());
    }
};

LM_COMP_REG_IMPL(DebugioContext_Client, "debugio::client");

// ------------------------------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(server)

class DebugioContext_Server final : public DebugioServerContext {
private:
    zmq::context_t context_;
    zmq::socket_t socket_;
    HandleMessageFunc on_handleMessage_;
    SyncUserContextFunc on_syncUserContext_;
    DrawFunc on_draw_;

public:
    DebugioContext_Server()
        : context_(1)
        , socket_(context_, ZMQ_REP)
    {}

public:
    virtual void on_handleMessage(const HandleMessageFunc& process) override { on_handleMessage_ = process; }
    virtual void on_syncUserContext(const SyncUserContextFunc& process) override { on_syncUserContext_ = process; }
    virtual void on_draw(const DrawFunc& process) override { on_draw_ = process; }

public:
    virtual bool construct(const Json& prop) override {
        // Waiting for the connection of the client
        try {
            socket_.bind(prop["address"]);
        }
        catch (const zmq::error_t& e) {
            LM_ERROR("ZMQ Error: {}", e.what());
            return false;
        }

        return true;
    }

    virtual void poll() override {
        zmq::pollitem_t item{ socket_, 0, ZMQ_POLLIN, 0 };
        zmq::poll(&item, 1, 0);
        if (item.revents & ZMQ_POLLIN) {
            processMessages();
        }
    }

    virtual void run() override {
        while (true) {
            processMessages();
        }
    }

private:
    void processMessages() {
        zmq::message_t ok;

        // Receive command
        zmq::message_t req_command;
        socket_.recv(req_command);
        const auto command = *req_command.data<Command>();
        socket_.send(ok, zmq::send_flags::none);

        // Receive arguments and execute the function
        zmq::message_t req_args;
        socket_.recv(req_args);
#if 0
        std::stringstream is(std::string(req_args.data<char>(), req_args.size()));
#else
        imemstream is(req_args.data<char>(), req_args.size());
#endif
        switch (command) {
            case Command::handleMessage: {
                std::string message;
                serial::load(is, message);
                on_handleMessage_(message);
                break;
            }
            case Command::syncUserContext: {
                lm::deserialize(is);
                on_syncUserContext_();
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

LM_COMP_REG_IMPL(DebugioContext_Server, "debugio::server");

LM_NAMESPACE_END(server)

// ------------------------------------------------------------------------------------------------

using ClientInstance = comp::detail::ContextInstance<DebugioContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    ClientInstance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    ClientInstance::shutdown();
}

LM_PUBLIC_API void handleMessage(const std::string& message) {
    if (ClientInstance::initialized()) {
        ClientInstance::get().handleMessage(message);
    }
}

LM_PUBLIC_API void syncUserContext() {
    if (ClientInstance::initialized()) {
        ClientInstance::get().syncUserContext();
    }
}

LM_PUBLIC_API void draw(int type, Vec3 color, const std::vector<Vec3>& vs) {
    if (ClientInstance::initialized()) {
        ClientInstance::get().draw(type, color, vs);
    }
}

LM_NAMESPACE_BEGIN(server)

using ServerInstance = comp::detail::ContextInstance<DebugioServerContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    ServerInstance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    ServerInstance::shutdown();
}

LM_PUBLIC_API void poll() {
    ServerInstance::get().poll();
}

LM_PUBLIC_API void run() {
    ServerInstance::get().run();
}

LM_PUBLIC_API void on_handleMessage(const HandleMessageFunc& process) {
    ServerInstance::get().on_handleMessage(process);
}

LM_PUBLIC_API void on_syncUserContext(const SyncUserContextFunc& process) {
    ServerInstance::get().on_syncUserContext(process);
}

LM_PUBLIC_API void on_draw(const DrawFunc& process) {
    ServerInstance::get().on_draw(process);
}

LM_NAMESPACE_END(server)
LM_NAMESPACE_END(LM_NAMESPACE::debugio)
