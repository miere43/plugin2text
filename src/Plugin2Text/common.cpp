#include "common.hpp"
#include "os.hpp"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <exception>

#ifdef _DEBUG
#pragma comment(lib, "zlibstatic-ngd.lib")
#else
#pragma comment(lib, "zlibstatic-ng.lib")
#endif

__declspec(allocator) void* tmpalloc_exec(Allocator& self_, MemoryOperation op, void* ptr, size_t size) {
    auto& self = (LinearAllocator&)self_;
    switch (op) {
        case MemoryOperation::Allocate: {
            verify(self.now + size <= self.end);
            auto result = self.now;
            self.now += size;// +(size % 16);
            return result;
        } break;

        case MemoryOperation::Free: {
        } break;

        default: {
            verify(false);
        } break;
    }

    return nullptr;
}

__declspec(allocator) void* stdalloc_exec(Allocator& self, MemoryOperation op, void* ptr, size_t size) {
    switch (op) {
        case MemoryOperation::Allocate: {
            return ::operator new(size);
        } break;

        case MemoryOperation::Free: {
            ::operator delete(ptr);
        } break;

        default: {
            verify(false);
        } break;
    }

    return nullptr;
};

LinearAllocator tmpalloc{ tmpalloc_exec };
Allocator stdalloc{ stdalloc_exec };

void memory_init() {
    auto data = allocate_virtual_memory(1024 * 1024 * 512);
    tmpalloc.start = data.start;
    tmpalloc.now = data.now;
    tmpalloc.end = data.end;
}

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
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

bool string_starts_with(const wchar_t* a, const wchar_t* b) {
    while (true) {
        if (*b == L'\0') {
            return true;
        } else if (*a != *b) {
            return false;
        }
        ++a;
        ++b;
    }
}

int string_index_of(const wchar_t* str, wchar_t c) {
    for (int i = 0; str[i]; ++i) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

wchar_t* substring(Allocator& allocator, const wchar_t* start, const wchar_t* end) {
    auto count = end - start;
    verify(count >= 0);
    auto buffer = (wchar_t*)memalloc(allocator, sizeof(wchar_t) * (count + 1));
    memcpy(buffer, start, count * sizeof(wchar_t));
    buffer[count] = L'\0';
    return buffer;
}

wchar_t* string_replace_extension(Allocator& allocator, const wchar_t* path, const wchar_t* new_extension) {
    int count = string_last_index_of(path, '.');
    if (count == -1) {
        count = static_cast<int>(wcslen(path));
    }

    const auto extension_count = wcslen(new_extension);
    const auto buffer = (wchar_t*)memalloc(allocator, sizeof(wchar_t) * (count + extension_count + 1));
    memcpy(buffer, path, count * sizeof(wchar_t));
    memcpy(&buffer[count], new_extension, extension_count * sizeof(wchar_t));
    buffer[count + extension_count] = L'\0';

    return buffer;
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

void* operator new(size_t size, Allocator& allocator) {
    return allocator.exec(allocator, MemoryOperation::Allocate, nullptr, size);
}

void operator delete(void* block, Allocator& allocator) {
    allocator.exec(allocator, MemoryOperation::Free, block, 0);
}

void* operator new[](size_t size, Allocator& allocator) {
    return allocator.exec(allocator, MemoryOperation::Allocate, nullptr, size);
}

void operator delete[](void* block, Allocator& allocator) {
    allocator.exec(allocator, MemoryOperation::Free, block, 0);
}

int String::index_of(const String& str) const {
    if (count >= str.count) {
        for (int i = 0; i < count - str.count; ++i) {
            if (0 == memcmp(&chars[i], str.chars, str.count)) {
                return i;
            }
        }
    }
    return -1;
}

void String::advance(int count) {
    verify(this->count >= count);
    this->chars += count;
    this->count -= count;
}

int String::compare(const String& rhs) const {
    return count == rhs.count ? strncmp(chars, rhs.chars, count) : count - rhs.count;
}
