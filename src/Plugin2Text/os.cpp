#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "common.hpp"
#include "os.hpp"

void* read_file(const wchar_t* path, uint32_t* size_out) {
    auto handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    verify(handle != INVALID_HANDLE_VALUE);

    uint64_t size;
    verify(GetFileSizeEx(handle, (LARGE_INTEGER*)&size));
    verify(size > 0 && size <= 0xffffffff);

    auto buffer = ::operator new(size);
    DWORD read = 0;
    verify(ReadFile(handle, buffer, (uint32_t)size, &read, nullptr));
    verify(read == size);

    CloseHandle(handle);
    if (size_out) {
        *size_out = read;
    }
    return buffer;
}

Slice allocate_virtual_memory(size_t size) {
    Slice slice;
    slice.start = (uint8_t*)VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    verify(slice.start);
    slice.now = slice.start;
    slice.end = slice.start + size;
    return slice;
}
