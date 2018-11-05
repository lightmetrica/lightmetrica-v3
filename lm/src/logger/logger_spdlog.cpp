/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE::log::detail)

class LoggerContext_spdlog : public LoggerContext {
private:
    int indentation_ = 0;
    std::string indentationString_;
    std::shared_ptr<spdlog::logger> stdoutLogger_;

public:
    virtual bool construct(const Json& prop) override {
        if (stdoutLogger_) { shutdown(); }
        #if 0
        stdoutLogger_ = spdlog::stdout_color_mt("lm_stdout");
        #else
        stdoutLogger_ = spdlog::stdout_logger_mt("lm_stdout");
        #endif
        stdoutLogger_->set_pattern("[%T.%e|%^%L%$] %v");
        return true;
    }

    void log(LogLevel level, const char* filename, int line, const char* message) {
        if (!stdoutLogger_) {
            return;
        }
        // Line and filename
        const auto lineAndFilename = fmt::format("{}@{}",
            line,
            std::filesystem::path(filename).stem().string());

        // Query log message
        stdoutLogger_->log(
            spdlog::level::level_enum(level),
            fmt::format("[{:<10}] {}{}", lineAndFilename.substr(0,10), indentationString_, message));
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
};

LM_COMP_REG_IMPL(LoggerContext_spdlog, "logger::spdlog");

LM_NAMESPACE_END(LM_NAMESPACE::log::detail)
