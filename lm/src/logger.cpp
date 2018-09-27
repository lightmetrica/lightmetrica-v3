/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::log::detail)

// ----------------------------------------------------------------------------

class Impl_ {
public:

    static Impl_& instance() {
        static Impl_ instance;
        return instance;
    }

public:

    void init() {
        if (init_) { shutdown(); }
        stdoutLogger_ = spdlog::stdout_color_mt("lm_stdout");
        init_ = true;
    }

    void shutdown() {
        stdoutLogger_ = nullptr;
        spdlog::shutdown();
        init_ = false;
    }

    void log(LogLevel level, const char* filename, int line, const char* message) {
        stdoutLogger_->log(spdlog::level::level_enum(level), indentationString_ + message);
    }

    void updateIndentation(int n) {
        indentation_ += n;
        if (indentation_ > 0) {
            indentationString_ = std::string(2 * indentation_, '.') + " ";
        }
        else {
            indentation_ = 0;
            indentationString_ = "";
        }
    }

private:

    bool init_ = false;
    int indentation_ = 0;
    std::string indentationString_;
    std::shared_ptr<spdlog::logger> stdoutLogger_;

};

// ----------------------------------------------------------------------------

LM_PUBLIC_API void init() {
    Impl_::instance().init();
}

LM_PUBLIC_API void shutdown() {
    Impl_::instance().shutdown();
}

LM_PUBLIC_API void log(LogLevel level, const char* filename, int line, const char* message) {
    Impl_::instance().log(level, filename, line, message);
}

LM_PUBLIC_API void updateIndentation(int n) {
    Impl_::instance().updateIndentation(n);
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE::log::detail)
