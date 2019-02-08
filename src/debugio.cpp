/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <zmq.hpp>
#include <lm/debugio.h>
#include <lm/serial.h>
#include <lm/logger.h>
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

// ----------------------------------------------------------------------------

// Shared command between server and client
enum class Command {
    handleMessage,
    syncUserContext,
    draw
};

// ----------------------------------------------------------------------------

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
        socket_.send(req_command);
        socket_.recv(&ok);

        // Send argument
        zmq::message_t req_args(serialized.data(), serialized.size());
        socket_.send(req_args);
        socket_.recv(&ok);
    }

public:
    virtual void handleMessage(const std::string& message) override {
        std::stringstream ss;
        serial::save(ss, message);
        call(Command::handleMessage, ss.str());
    }

    virtual void syncUserContext() override {
        // Serialize the internal state
        std::stringstream ss;
        lm::serialize(ss);
        call(Command::syncUserContext, ss.str());
    }

    virtual void draw(int type, const std::vector<Vec3>& vs) override {
        std::stringstream ss;
        serial::save(ss, type, vs);
        call(Command::draw, ss.str());
    }
};

LM_COMP_REG_IMPL(DebugioContext_Client, "debugio::client");

// ----------------------------------------------------------------------------

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
        const int port = prop["port"];
        try {
            socket_.bind(fmt::format("tcp://*:{}", port));
        }
        catch (const zmq::error_t& e) {
            LM_ERROR("ZMQ Error: {}", e.what());
            return false;
        }

        return true;
    }

    virtual void run() override {
        while (true) {
            zmq::message_t ok;

            // Receive command
            zmq::message_t req_command;
            socket_.recv(&req_command);
            const auto command = *req_command.data<Command>();
            socket_.send(ok);

            // Receive arguments and execute the function
            zmq::message_t req_args;
            socket_.recv(&req_args);
            imemstream is(req_args.data<char>(), req_args.size());
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
                    std::vector<Vec3> vs;
                    serial::load(is, type, vs);
                    on_draw_(type, vs);
                    break;
                }
            }
            socket_.send(ok);
        }
    }
};

LM_COMP_REG_IMPL(DebugioContext_Server, "debugio::server");

LM_NAMESPACE_END(server)

// ----------------------------------------------------------------------------

using Instance = comp::detail::ContextInstance<Component>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void handleMessage(const std::string& message) {
    Instance::get().cast<DebugioContext>()->handleMessage(message);
}

LM_PUBLIC_API void syncUserContext() {
    Instance::get().cast<DebugioContext>()->syncUserContext();
}

LM_PUBLIC_API void draw(int type, const std::vector<Vec3>& vs) {
    Instance::get().cast<DebugioContext>()->draw(type, vs);
}

LM_NAMESPACE_BEGIN(server)

LM_PUBLIC_API void run() {
    Instance::get().cast<DebugioServerContext>()->run();
}

LM_PUBLIC_API void on_handleMessage(const HandleMessageFunc& process) {
    Instance::get().cast<DebugioServerContext>()->on_handleMessage(process);
}

LM_PUBLIC_API void on_syncUserContext(const SyncUserContextFunc& process) {
    Instance::get().cast<DebugioServerContext>()->on_syncUserContext(process);
}

LM_PUBLIC_API void on_draw(const DrawFunc& process) {
    Instance::get().cast<DebugioServerContext>()->on_draw(process);
}

LM_NAMESPACE_END(server)
LM_NAMESPACE_END(LM_NAMESPACE::debugio)
