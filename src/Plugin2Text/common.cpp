#include "common.hpp"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

__declspec(noreturn) void exit_error(const wchar_t* format, ...) {
    wprintf(L"error: ");
    va_list args;
    va_start(args, format);
    vwprintf(format, args);
    va_end(args);
    exit(1);
}

bool string_equals(const wchar_t* a, const wchar_t* b) {
    if (a == b) return true;
    if (a == nullptr || b == nullptr) return false;
    return 0 == wcscmp(a, b);
}

bool memory_equals(const void* a, const void* b, size_t size){
    return 0 == memcmp(a, b, size);
}

int string_last_index_of(const wchar_t* str, char c) {
    const auto count = str ? (int)wcslen(str) : 0;
    for (int i = count - 1; i >= 0; --i) {
        if (str[i] == '.') {
            return i;
        }
    }
    return -1;
}