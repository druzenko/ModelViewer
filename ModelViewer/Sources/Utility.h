#pragma once

#include "pch.h"
#include <winerror.h>

namespace Utility
{
    template<typename T>
    concept Printable = std::is_same_v<typename std::remove_const<T>::type, char> || std::is_same_v<typename std::remove_const<T>::type, wchar_t>;

    template<Printable T>
    inline void Print(const T* message)
    {
        if constexpr (std::is_same_v<typename std::remove_const<T>::type, char>)
        {
            OutputDebugStringA(message);
            OutputDebugStringA("\n");
        }
        else
        {
            OutputDebugString(message);
            OutputDebugString(L"\n");
        }
    }

    template<Printable T>
    inline void Printf(const T* format, ...)
    {
        using charType = std::remove_const<T>::type;
        charType buffer[256];
        va_list ap;
        va_start(ap, format);

        if constexpr (std::is_same_v<charType, char> == true)
        {
            vsprintf_s(buffer, 256, format, ap);
        }
        else
        {
            vswprintf(buffer, 256, format, ap);
        }

        va_end(ap);
        Print(buffer);
    }

#ifdef ASSERT
#undef ASSERT
#endif

#ifndef RELEASE
    template<Printable T>
    inline void PrintSubMessage(const T* format, ...)
    {
        Print("--> ");
        using charType = std::remove_const<T>::type;
        charType buffer[256];
        va_list ap;
        va_start(ap, format);
        if constexpr (std::is_same_v<charType, char> == true)
        {
            vsprintf_s(buffer, 256, format, ap);
        }
        else
        {
            vswprintf(buffer, 256, format, ap);
        }
        
        va_end(ap);
        Print(buffer);
        Print("\n");
    }

#define STRINGIFY(x) #x
#define STRINGIFY_BUILTIN(x) STRINGIFY(x)
#define ASSERT( isFalse, ... ) \
        if (!(bool)(isFalse)) { \
            Utility::Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("\'" #isFalse "\' is false"); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
            __debugbreak(); \
        }

#define ASSERT_HRESULT( hr, ... ) \
        if (FAILED(hr)) { \
            Utility::Print("\nHRESULT failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("hr = 0x%08X", hr); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
            Graphics::GPUCrashCallback(hr); \
            __debugbreak(); \
        }

#define WARN_ONCE_IF( isTrue, ... ) \
    { \
        static bool s_TriggeredWarning = false; \
        if ((bool)(isTrue) && !s_TriggeredWarning) { \
            s_TriggeredWarning = true; \
            Utility::Print("\nWarning issued in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("\'" #isTrue "\' is true"); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
        } \
    }

#define WARN_ONCE_IF_NOT( isTrue, ... ) WARN_ONCE_IF(!(isTrue), __VA_ARGS__)

#endif
}