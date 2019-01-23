/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/lm.h>

class LoggerContext_User final : public lm::log::detail::LoggerContext {
    void log(lm::log::LogLevel, int, const char*, int, const char* message) override {
        std::cout << "[user] " << message << std::endl;
    }
    void updateIndentation(int) override {}
    void setSeverity(int) override {}
};

LM_COMP_REG_IMPL(LoggerContext_User, "logger::user");

/*
    This example illustrates the usage of logger system.
*/
int main() {
    try {
        // Parameters for logger can be passed via lm::init() function.
        // Default logger type `logger::default` outputs the logs into the terminal.
        lm::init("user::default", {
            {"logger", "logger::default"}
        });

        // Log messages has its own severity level.
        // We provide several macros to output the messages of the correspoinding severity.
        LM_INFO("Info message");
        LM_WARN("Warning message");
        LM_ERROR("Error message");
        
        // Some loggers supports indentation of log message so that we can organize the messages.
        // LM_INDENT() macro automatically increase and decrease indentation inside a space.
        LM_INFO("Indent 0");
        {
            LM_INDENT();
            LM_INFO("Indent 1");
            {
                LM_INDENT();
                LM_INFO("Indent 2");
            }
            LM_INFO("Indent 1");
        }

        // Multiline string is also supported.
        lm::Json json{
            {"a", 1},
            {"b", 2},
            {"c", {
                {"c1", 3},
                {"c1", 4}
            }}
        };
        LM_INFO(json.dump());
        LM_INFO(json.dump(2));

        // Controlling severity
        lm::log::setSeverity(lm::log::LogLevel::Warn);
        LM_INFO("Info message");
        LM_WARN("Warning message");
        LM_ERROR("Error message");
        lm::log::setSeverity(lm::log::LogLevel::Err);
        LM_INFO("Info message");
        LM_WARN("Warning message");
        LM_ERROR("Error message");

        // User-defined logger
        lm::init("user::default", {
            {"logger", "logger::user"}
        });
        LM_INFO("Info message");
        LM_WARN("Warning message");
        LM_ERROR("Error message");
    }
    catch (const std::exception& e) {
        LM_ERROR("Runtime error: {}", e.what());
    }

    return 0;
}