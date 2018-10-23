#include "log.hpp"
#include <cstdio>
#include <cstdarg>
#include <atomic>
#include <iostream>
#include <mutex>
#if _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace iris
{
    std::atomic_bool is_initialized{false};
    bool is_console = false;
#if _WIN32
    HANDLE  hstdout = 0;
    WORD    default_attributes;
#endif

    void ensure_initialized()
    {
        if (!is_initialized.load())
        {
#if _WIN32
            hstdout = ::GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO info;
            is_console = !!::GetConsoleScreenBufferInfo(hstdout, &info);
            default_attributes = info.wAttributes;
#else
            is_console = isatty(fileno(stdout));
#endif
            is_initialized.store(true);
        }
    }

    std::mutex s_logmutex;

    void log(log_level lv, bool asline, int line, const char* file, const char * fmt, ...)
    {
        std::lock_guard<std::mutex> lock(s_logmutex);
        ensure_initialized();
        static char log_buf[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(log_buf, 2048, fmt, args);
        va_end(args);
        switch (lv)
        {
        case log_warning:
            config_console_color(cc_yellow);
            break;
        case log_error:
        case log_fatal:
            config_console_color(cc_red);
            break;
        case log_info:
        default:
            config_console_color(cc_default);
            break;
        }
        std::cout << log_buf;
        if(asline)
            std::cout << std::endl;
        config_console_color(cc_default);
    }

    void log(const char* fmt, ...)
    {
        std::lock_guard<std::mutex> lock(s_logmutex);
        ensure_initialized();
        static char log_buf1[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(log_buf1, 2048, fmt, args);
        va_end(args);
        std::cout << log_buf1;
    }

    void config_console_color(console_color c)
    {
        ensure_initialized();
        switch (c)
        {
        case cc_default:
#if _WIN32
            ::SetConsoleTextAttribute(hstdout, default_attributes);
#endif
            break;
        case cc_red:
#if _WIN32
            ::SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
#endif
            break;
        case cc_yellow:
#if _WIN32
            ::SetConsoleTextAttribute(hstdout, FOREGROUND_RED | FOREGROUND_GREEN);
#endif
            break;
        case cc_green:
#if _WIN32
            ::SetConsoleTextAttribute(hstdout, FOREGROUND_GREEN);
#endif
            break;
        case cc_blue:
#if _WIN32
            ::SetConsoleTextAttribute(hstdout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
            break;
        default:
            break;
        }
    }
}
