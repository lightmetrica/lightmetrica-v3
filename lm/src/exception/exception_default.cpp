/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include <pch.h>
#include <lm/exception.h>
#include <lm/logger.h>

#if LM_PLATFORM_WINDOWS
#include <Windows.h>
#include <eh.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
#endif

#define LM_EXCEPTION_ERROR_CODE(m, code) m[code] = #code

LM_NAMESPACE_BEGIN(LM_NAMESPACE::exception::detail)

class ExceptionContext_Default : public ExceptionContext {
private:


public:
    ExceptionContext_Default() {
        #if LM_PLATFORM_WINDOWS
        _set_se_translator([](unsigned int code, PEXCEPTION_POINTERS data) {
            // Map of the error code descriptions
            std::unordered_map<unsigned int, std::string> m;
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_ACCESS_VIOLATION);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_DATATYPE_MISALIGNMENT);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_BREAKPOINT);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_SINGLE_STEP);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_FLT_DENORMAL_OPERAND);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_FLT_DIVIDE_BY_ZERO);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_FLT_INEXACT_RESULT);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_FLT_INVALID_OPERATION);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_FLT_OVERFLOW);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_FLT_STACK_CHECK);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_FLT_UNDERFLOW);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_INT_DIVIDE_BY_ZERO);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_INT_OVERFLOW);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_PRIV_INSTRUCTION);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_IN_PAGE_ERROR);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_ILLEGAL_INSTRUCTION);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_NONCONTINUABLE_EXCEPTION);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_STACK_OVERFLOW);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_INVALID_DISPOSITION);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_GUARD_PAGE);
            LM_EXCEPTION_ERROR_CODE(m, EXCEPTION_INVALID_HANDLE);

            // Print error message
            LM_ERROR("Structured exception [desc='{}']", m[code]);
            LM_ERROR("Stack trace");
            LM_INDENT();
            stackTrace_();

            throw std::runtime_error(m[code]);
        });
        enableFPEx();
        #endif
    }

    ~ExceptionContext_Default() {
        #if LM_PLATFORM_WINDOWS
        disableFPEx();
        _set_se_translator(nullptr);
        #endif
    }

private:
    unsigned int setFPExState(unsigned int state) const {
        // Get current floating-point control word
        unsigned int old;
        _controlfp_s(&old, 0, 0);
        // Set a new control word
        unsigned int current;
        _controlfp_s(&current, state, _MCW_EM);
        LM_UNUSED(current);
        return old;
    }

private:

public:
    virtual bool construct(const Json& prop) override {
        LM_UNUSED(prop);
        return true;
    }

    virtual void enableFPEx() override {
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
        setFPExState(~(_EM_INVALID | _EM_ZERODIVIDE));
    }

    virtual void disableFPEx() override {
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);
        setFPExState(_CW_DEFAULT);
    }

    virtual void stackTrace() override {
        stackTrace_();
    }

    static void stackTrace_() {
        #if LM_PLATFORM_WINDOWS
        // Get necessary function
        using CaptureStackBackTraceFunc= USHORT(WINAPI*)(__in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG);
        auto RtlCaptureStackBackTrace = CaptureStackBackTraceFunc(
            GetProcAddress(LoadLibrary("kernel32.dll"), "RtlCaptureStackBackTrace"));
    
        // Prepare for capturing symbols
        auto process = GetCurrentProcess();
        SymInitialize(process, NULL, TRUE);

        // Allocate symbol information
        SYMBOL_INFO* symbol;
        symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

        // Print captured stack frame
        constexpr int MaxCallers = 62;
        void* callers_stack[MaxCallers];
        const int frames = RtlCaptureStackBackTrace(0, MaxCallers, callers_stack, NULL);
        for (int i = 0; i < std::min(frames, 10); i++) {
            SymFromAddr(process, (DWORD64)(callers_stack[i]), 0, symbol);
            LM_ERROR("{}: {} {} - {}", i, callers_stack[i], symbol->Name, symbol->Address);
        }
        free(symbol);
        #endif
    }
};

LM_COMP_REG_IMPL(ExceptionContext_Default, "exception::default");

LM_NAMESPACE_END(LM_NAMESPACE::exception::detail)
