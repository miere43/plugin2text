#include "common.hpp"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

__declspec(noreturn) void exit_error(const wchar_t* format, ...) {
    wprintf(L"error: ");
    va_list args;
    va_start(args, format);
    vwprintf(format, args);
    va_end(args);
    exit(1);
}
