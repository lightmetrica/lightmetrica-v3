/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/core.h>
#include <lm/debug.h>
#include <lm/parallel.h>
#if LM_PLATFORM_WINDOWS
#include <Windows.h>
#endif

LM_NAMESPACE_BEGIN(LM_NAMESPACE::debug)

struct Context {
    OnPollFunc on_poll;
    static Context& instance() {
        static Context instance;
        return instance;
    };
};

// ------------------------------------------------------------------------------------------------

LM_PUBLIC_API void poll(const Json& val) {
    auto& on_poll = Context::instance().on_poll;
    if (on_poll) {
        on_poll(val);
    }
}

LM_PUBLIC_API void reg_on_poll(const OnPollFunc& func) {
    Context::instance().on_poll = func;
}

// ------------------------------------------------------------------------------------------------

LM_PUBLIC_API void attach_to_debugger() {
    #if LM_PLATFORM_WINDOWS
    // cf. https://stackoverflow.com/questions/20337870/what-is-the-equivalent-of-system-diagnostics-debugger-launch-in-unmanaged-code

    // Get Windows system directory
    std::wstring systemDir(MAX_PATH + 1, '\0');
    auto nc = GetSystemDirectoryW(&systemDir[0], UINT(systemDir.length()));
    if (nc == 0) {
        LM_ERROR("Failed to get system directory");
        return;
    }
    systemDir.resize(nc);

    // Get process ID and create the command line
    DWORD pid = GetCurrentProcessId();
    std::wostringstream s;
    s << systemDir << L"\\vsjitdebugger.exe -p " << pid;
    std::wstring cmdLine = s.str();

    // Start debugger process
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        LM_ERROR("Failed to launch vsjitdebugger.exe");
        return;
    }

    // Close debugger process handles to eliminate resource leak
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // Wait for the debugger to attach
    while (!IsDebuggerPresent()) {
        Sleep(100);
    }
    #else
    LM_THROW_EXCEPTION(Error::Unsupported, "attachToDebugger() is not supported in this platform.");
    #endif
}

LM_PUBLIC_API void print_asset_tree(bool visualize_weak_refs) {
    using namespace std::placeholders;

    // Traverse the asset from the root
    using Func = std::function<void(Component * &p, bool weak, std::string parent_loc)>;
    const Func visitor = [&](Component*& comp, bool weak, std::string parent_loc) {
        if (!comp) {
            return;
        }

        if (weak) {
            if (!visualize_weak_refs) {
                return;
            }

            // Print information of weak reference
            LM_INFO("-> {}", comp->loc());
        }
        else {
            // Current component index
            const auto loc = comp->loc();
            auto comp_id = loc;
            comp_id.erase(0, parent_loc.size());

            // Print information of owned pointer
            LM_INFO("{} [{}]", comp_id, comp->key());
            LM_INDENT();
            
            // Traverse underlying components
            comp->foreach_underlying(std::bind(visitor, _1, _2, loc));
        }
    };
        
    LM_INFO("$.assets");
    LM_INDENT();
    auto* assets = lm::comp::get<Component>("$.assets");
    assets->foreach_underlying(std::bind(visitor, _1, _2, "$.assets"));
}

LM_NAMESPACE_END(LM_NAMESPACE::debug)
