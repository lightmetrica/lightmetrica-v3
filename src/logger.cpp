/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/logger.h>
#include <lm/json.h>
#include "ext/rang.hpp"

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::log::detail)

class LoggerContext_Default : public LoggerContext {
private:
    #if LM_DEBUG_MODE
    int severity_ = -100;
    #else
    int severity_ = 0;
    #endif
    int indentation_ = 0;
    std::string indentationString_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_ =
        std::chrono::high_resolution_clock::now();
    std::mutex mutex_;
    bool color_;

    struct Style {
        std::string name;
        rang::fg color;
    };
    std::unordered_map<LogLevel, Style> styles_{
        {LogLevel::Debug      , {"D", rang::fg::cyan}},
        {LogLevel::Info       , {"I", rang::fg::green}},
        {LogLevel::Warn       , {"W", rang::fg::yellow}},
        {LogLevel::Err        , {"E", rang::fg::red}},
        {LogLevel::Progress   , {"I", rang::fg::green}},
        {LogLevel::ProgressEnd, {"I", rang::fg::green}},
    };

public:
    virtual bool construct(const Json& prop) override {
        color_ = json::value(prop, "color", true);
        return true;
    }

public:
    void log(LogLevel level, int severity, const char* filename, int line, const char* message) {
        // Ignore message if the current severity is lower than that of message
        if (severity_ > severity) {
            return;
        }

        std::unique_lock<std::mutex> lock(mutex_);

        // Elapsed time
        const auto now = std::chrono::high_resolution_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);

        // Line and filename
        const auto lineAndFilename = fmt::format("{}@{}",
            line,
            std::filesystem::path(filename).stem().string());

        // Style
        const auto style = styles_[level];

        // Formatted message
        // <message> = <header> <body><eol><whitespaces>
        // <header>  = [{<log type>|<elapsed>|<line>@<filename>]
        // <body>    = <indentations><message>
        const auto header = fmt::format("[{}|{:.3f}|{:<10}] ",
            style.name, elapsed.count() / 1000.0,
            lineAndFilename.substr(0, 10));

        // Case with progress message
        // Progress message does not support multiline message.
        if (level == LogLevel::Progress || level == LogLevel::ProgressEnd) {
            // Message with indentation
            const auto body = fmt::format("{}{}",
                indentationString_,
                message);

            // Calculate white spaces to hide already written message if necessary.
            // Keep track of maximum message length for progress outputs.
            static int maxProgressMessageLen = 0;
            static std::string whitespaces;
            if (level == LogLevel::Progress || level == LogLevel::ProgressEnd) {
                // Current message length
                const int messageLen = int(header.size() + body.size());

                // If the current message is shorter than maximum,
                // hide the message with white spaces.
                if (maxProgressMessageLen > messageLen) {
                    whitespaces.assign(maxProgressMessageLen - messageLen, ' ');
                }
                else {
                    maxProgressMessageLen = messageLen;
                }
            }

            // Print formatted log message
            if (color_) {
                std::cout << style.color << header << rang::style::reset;
            }
            else {
                std::cout << header;
            }
            std::cout << body << whitespaces << "\r" << std::flush;

            // ProgressEnd incurs newline so reset the cached values
            if (level == LogLevel::ProgressEnd) {
                maxProgressMessageLen = 0;
                whitespaces.clear();
            }
        }
        // Other message types
        else {
            // Process one line
            const auto processLine = [&](const std::string& message) {
                // Message with indentation
                const auto body = fmt::format("{}{}",
                    indentationString_,
                    message);

                // Print formatted log message
                if (color_) {
                    std::cout << style.color << header << rang::style::reset;
                }
                else {
                    std::cout << header;
                }
                std::cout << body << "\n" << std::flush;
            };

            if (strlen(message) == 0) {
                // Empty line
                processLine("");
            }
            else {
                // Split the mesagge by lines
                std::stringstream ss(message);
                std::string messageByLine;
                while (std::getline(ss, messageByLine, '\n')) {
                    processLine(messageByLine);
                }
            }
        }
    }

    void updateIndentation(int n) {
        std::unique_lock<std::mutex> lock(mutex_);
        indentation_ += n;
        if (indentation_ > 0) {
            indentationString_ = std::string(2 * indentation_, '.') + " ";
        }
        else {
            indentation_ = 0;
            indentationString_ = "";
        }
    }

    void setSeverity(int severity) override {
        severity_ = severity;
    }
};

LM_COMP_REG_IMPL(LoggerContext_Default, "logger::default");

LM_NAMESPACE_END(LM_NAMESPACE::log::detail)

// ----------------------------------------------------------------------------

LM_NAMESPACE_BEGIN(LM_NAMESPACE::log)

using Instance = comp::detail::ContextInstance<detail::LoggerContext>;

LM_PUBLIC_API void init(const std::string& type, const Json& prop) {
    Instance::init(type, prop);
}

LM_PUBLIC_API void shutdown() {
    Instance::shutdown();
}

LM_PUBLIC_API void setSeverity(int severity) {
    if (Instance::initialized()) {
        Instance::get().setSeverity(severity);
    }
}

LM_PUBLIC_API void log(LogLevel level, int severity, const char* filename, int line, const char* message) {
    if (Instance::initialized()) {
        Instance::get().log(level, severity, filename, line, message);
    }
    else {
        // Fallback to stdout
        std::cout << message << std::endl;
    }
}

LM_PUBLIC_API void updateIndentation(int n) {
    if (Instance::initialized()) {
        Instance::get().updateIndentation(n);
    }
}

LM_NAMESPACE_END(LM_NAMESPACE::log)
