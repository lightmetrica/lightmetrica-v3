/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/ansicolor_sink.h>

#if LM_PLATFORM_WINDOWS
#include <Windows.h>
#endif

LM_NAMESPACE_BEGIN(LM_NAMESPACE::log::detail)

class LoggerContext_spdlog : public LoggerContext {
private:
    int indentation_ = 0;
    std::string indentationString_;
    std::shared_ptr<spdlog::logger> stdoutLogger_;
    std::shared_ptr<spdlog::logger> progressLogger_;
    bool prevIsProgressMessage_ = false;  // Previous log message is progress

public:
    LoggerContext_spdlog() {
        // Create logger
        // Always use ansicolor_stdout_sink even in Windows because Windows 10 supports ANSI color codes.
        const auto createLogger = [](const std::string& name) {
            return spdlog::default_factory::template create<spdlog::sinks::ansicolor_stdout_sink_mt>(name);
        };
        stdoutLogger_   = createLogger("lm_stdout");
        progressLogger_ = createLogger("lm_progress");

        // Set pattern
        const std::string pattern = "[%T.%e|%^%L%$] %v";
        stdoutLogger_->set_pattern(pattern);

        // Set the log level that triggers automatic flush
        stdoutLogger_->flush_on(spdlog::level::err);
        progressLogger_->flush_on(spdlog::level::info);

        // Disable automatic newline
        auto formatter = std::make_unique<spdlog::pattern_formatter>(pattern + "\r", spdlog::pattern_time_type::local, "");
        progressLogger_->set_formatter(std::move(formatter));
    }

    ~LoggerContext_spdlog() {
        spdlog::shutdown();
    }

public:
    void log(LogLevel level, const char* filename, int line, const char* message) {
        if (!stdoutLogger_) {
            return;
        }
        
        #if 0
        // If the previous message is progress message, fill the line with white spaces
        if (prevIsProgressMessage_) {
            // Get the width of the current console
            const int consoleWidth = []() {
                constexpr int DefaultConsoleWidth = 50;
                #if LM_PLATFORM_WINDOWS
	            HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	            CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo;
                return GetConsoleScreenBufferInfo(consoleHandle, &screenBufferInfo)
                    ? screenBufferInfo.dwSize.X - 1
                    : DefaultConsoleWidth;
                #elif LM_PLATFORM_LINUX || LM_PLATFORM_APPLE
	            struct winsize w;
                return ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != 0
                    ? w.ws_col
                    : DefaultConsoleWidth;
                #endif
            }();
            std::cout << std::string(consoleWidth, ' ');
            std::cout.flush();
            std::cout << "\r";
            std::cout.flush();
        }
        #endif

        // Line and filename
        const auto lineAndFilename = fmt::format("{}@{}",
            line,
            std::filesystem::path(filename).stem().string());

        // Query log message
        const auto formatted = fmt::format("[{:<10}] {}{}", lineAndFilename.substr(0, 10), indentationString_, message);
        if (level == LogLevel::Progress) {
            progressLogger_->log(spdlog::level::info, formatted);
            prevIsProgressMessage_ = true;
        }
        else {
            stdoutLogger_->log(spdlog::level::level_enum(level), formatted);
            prevIsProgressMessage_ = false;
        }
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
