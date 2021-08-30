#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include "common.hpp"
#include "os.hpp"

StaticArray<uint8_t> read_file(const wchar_t* path) {
    auto handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    verify(handle != INVALID_HANDLE_VALUE);

    uint64_t size;
    verify(GetFileSizeEx(handle, (LARGE_INTEGER*)&size));
    verify(size > 0 && size <= 0xffffffff);

    auto buffer = new uint8_t[size];
    DWORD read = 0;
    verify(ReadFile(handle, buffer, (uint32_t)size, &read, nullptr));
    verify(read == size);

    CloseHandle(handle);
    return { buffer, read };
}

Slice allocate_virtual_memory(size_t size) {
    Slice slice;
    slice.start = (uint8_t*)VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    verify(slice.start);
    slice.now = slice.start;
    slice.end = slice.start + size;
    return slice;
}

void free_virtual_memory(Slice* slice) {
    verify(slice);
    verify(VirtualFree(slice->start, 0, MEM_RELEASE));
    *slice = Slice();
}

void write_file(const wchar_t* path, const StaticArray<uint8_t>& data) {
    auto handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    verify(handle != INVALID_HANDLE_VALUE);
    verify(data.count <= 0xffffffff);

    DWORD written = 0;
    verify(WriteFile(handle, data.data, (DWORD)data.count, &written, nullptr));
    verify(written == (DWORD)data.count);

    CloseHandle(handle);
}

wchar_t* const* get_command_line_args(int* argc) {
    auto result = CommandLineToArgvW(GetCommandLineW(), argc);
    verify(result);
    return result;
}
