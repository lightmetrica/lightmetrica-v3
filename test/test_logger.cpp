/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/loggercontext.h>

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

TEST_CASE("Logger") {
    lm::log::ScopedInit log_("logger::default", {
        {"color", false}
    });

    // --------------------------------------------------------------------------------------------

    const auto extractMessage = [](const std::string& out) -> std::string {
        std::regex re(R"x(^\[.*\] +(.*)\n?)x");
        std::smatch match;
        if (!std::regex_match(out, match, re)) {
            return "";
        }
        return std::string(match[1]);
    };

    const auto nextMessage = [&](std::stringstream& ss) -> std::string
    {
        std::string line;
        std::getline(ss, line, '\n');
        return extractMessage(line);
    };

    // --------------------------------------------------------------------------------------------

    SUBCASE("Log messages different severity levels") {
        // Log messages has its own severity level.
        // We provide several macros to output the messages of the correspoinding severity.

        std::string out;
        out = extractMessage(captureStdout([]() { LM_INFO("Info"); }));
        CHECK(out == "Info");
        out = extractMessage(captureStdout([]() { LM_WARN("Warning"); }));
        CHECK(out == "Warning");
        out = extractMessage(captureStdout([]() { LM_ERROR("Error"); }));
        CHECK(out == "Error");
    }

    SUBCASE("Indentation") {
        // Some loggers supports indentation of log message so that we can organize the messages.
        // LM_INDENT() macro automatically increase and decrease indentation inside a space.

        const auto out = captureStdout([]() {
            LM_INFO("Indent 0");
            LM_INDENT();
            LM_INFO("Indent 1");
            {
                LM_INDENT();
                LM_INFO("Indent 2");
            }
            LM_INFO("Indent 1");
        });
        
        std::stringstream ss(out);
        CHECK(nextMessage(ss) == "Indent 0");
        CHECK(nextMessage(ss) == ".. Indent 1");
        CHECK(nextMessage(ss) == ".... Indent 2");
        CHECK(nextMessage(ss) == ".. Indent 1");
    }

    SUBCASE("Multiline") {
        // Multiline string is also supported.
        const auto out = captureStdout([]() {
            lm::Json json{
                {"a", 1},
                {"b", 2},
                {"c", {
                    {"c1", 3},
                    {"c1", 4}
                }}
            };
            LM_INFO(json.dump(2));
        });
        std::stringstream ss(out);
        CHECK(nextMessage(ss) == R"({)");
        CHECK(nextMessage(ss) == R"("a": 1,)");
        CHECK(nextMessage(ss) == R"("b": 2,)");
        CHECK(nextMessage(ss) == R"("c": {)");
        CHECK(nextMessage(ss) == R"("c1": 3)");
        CHECK(nextMessage(ss) == R"(})");
        CHECK(nextMessage(ss) == R"(})");
    }

    SUBCASE("Controlling severity") {
        {
            const auto out = captureStdout([]() {
                lm::log::set_severity(lm::log::LogLevel::Warn);
                LM_INFO("Info");
                LM_WARN("Warning");
                LM_ERROR("Error");
            });
            std::stringstream ss(out);
            CHECK(nextMessage(ss) == "Warning");
            CHECK(nextMessage(ss) == "Error");
        }
        {
            const auto out = captureStdout([]() {
                lm::log::set_severity(lm::log::LogLevel::Err);
                LM_INFO("Info");
                LM_WARN("Warning");
                LM_ERROR("Error");
            });
            std::stringstream ss(out);
            CHECK(nextMessage(ss) == "Error");
        }
    }

    SUBCASE("User-defined severity") {
        {
            const auto out = captureStdout([]() {
                lm::log::set_severity(10);
                LM_LOG(10, "Severity 10");
                LM_LOG(20, "Severity 20");
                LM_LOG(30, "Severity 30");
            });
            std::stringstream ss(out);
            CHECK(nextMessage(ss) == "Severity 10");
            CHECK(nextMessage(ss) == "Severity 20");
            CHECK(nextMessage(ss) == "Severity 30");
        }
        {
            const auto out = captureStdout([]() {
                lm::log::set_severity(20);
                LM_LOG(10, "Severity 10");
                LM_LOG(20, "Severity 20");
                LM_LOG(30, "Severity 30");
            });
            std::stringstream ss(out);
            CHECK(nextMessage(ss) == "Severity 20");
            CHECK(nextMessage(ss) == "Severity 30");
        }
    }
}

class LoggerContext_User final : public lm::log::LoggerContext {
    void log(lm::log::LogLevel, int, const char*, int, const char* message) override {
        std::cout << "[user] " << message << std::endl;
    }
    void update_indentation(int) override {}
    void set_severity(int) override {}
};

LM_COMP_REG_IMPL(LoggerContext_User, "logger::user");

TEST_CASE("User-defined logger") {
    lm::log::ScopedInit log_("logger::user");
    std::string out;
    out = captureStdout([]() { LM_INFO("Info"); });
    CHECK(out == "[user] Info\n");
    out = captureStdout([]() { LM_WARN("Warning"); });
    CHECK(out == "[user] Warning\n");
    out = captureStdout([]() { LM_ERROR("Error"); });
    CHECK(out == "[user] Error\n");
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
