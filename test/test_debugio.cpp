/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include "test_common.h"
#include <lm/debugio.h>
using namespace std::literals::chrono_literals;

LM_NAMESPACE_BEGIN(LM_TEST_NAMESPACE)

TEST_CASE("Debugio") {
    lm::log::ScopedInit log_;

    // Create two threads and simulate communication via socket

    // Server
    std::thread thread_server([]() {
        lm::debugio::server::ScopedInit init("debugio::server", {
            {"address", "tcp://*:5555"}
        });
        std::atomic<bool> done = false;
        lm::debugio::server::on_handle_message([&](const std::string& message) {
            CHECK(message == "hai domo");
            done = true;
        });
        while (!done) {
            lm::debugio::server::poll();
        }
    });

    // Client
    std::thread thread_client([]() {
        lm::debugio::ScopedInit init("debugio::client", {
            {"address", "tcp://localhost:5555"}
        });

        // Send a message to the server
        lm::debugio::handle_message("hai domo");
    });

    thread_client.join();
    thread_server.join();
}

LM_NAMESPACE_END(LM_TEST_NAMESPACE)
