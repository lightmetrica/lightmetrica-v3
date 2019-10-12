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

class Context {
public:
    static Context& instance() {
        static Context instance;
        return instance;
    }

private:
    OnPollFloatFunc onPollFloat_;

public:
    void pollFloat(const std::string& name, Float val) {
        if (!parallel::mainThread()) {
            return;
        }
        if (!onPollFloat_) {
            LM_THROW_EXCEPTION(Error::None, "onPollFloat function is not registered.");
        }
        onPollFloat_(name, val);
    }

    void regOnPollFloat(const OnPollFloatFunc& onPollFloat) {
        onPollFloat_ = onPollFloat;
    }
};

// ------------------------------------------------------------------------------------------------

LM_PUBLIC_API void pollFloat(const std::string& name, Float val) {
    Context::instance().pollFloat(name, val);
}

LM_PUBLIC_API void regOnPollFloat(const OnPollFloatFunc& onPollFloat) {
    Context::instance().regOnPollFloat(onPollFloat);
}

LM_PUBLIC_API void attachToDebugger() {
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

LM_NAMESPACE_END(LM_NAMESPACE::debug)
