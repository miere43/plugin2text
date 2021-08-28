#include "common.hpp"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <exception>

#ifdef _DEBUG
#pragma comment(lib, "zlibstaticd.lib")
#else
#pragma comment(lib, "zlibstatic.lib")
#endif

__declspec(noreturn) void verify_impl(const char* msg, const char* file, int line) {
    printf("error: assertion failed: condition \"%s\" is false (%s:%d)\n", msg, file, line);
    throw std::exception("err");
}

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

void BinaryReader::read(void* out, size_t size) {
    verify(now + size <= end);
    memcpy(out, now, size);
    now += size;
}

const void* BinaryReader::advance(size_t size) {
    verify(now + size <= end);
    auto result = now;
    now += size;
    return result;
}

const WString* BinaryReader::advance_wstring() {
    auto str = (const WString*)advance(sizeof(uint16_t));
    advance(str->count);
    return str;
}
