#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj_core.h>
#include "common.hpp"
#include "os.hpp"
#include <PathCch.h>

#pragma comment(lib, "pathcch.lib")

StaticArray<uint8_t> read_file(Allocator& allocator, const wchar_t* path) {
    auto handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    verify(handle != INVALID_HANDLE_VALUE);

    uint64_t size;
    verify(GetFileSizeEx(handle, (LARGE_INTEGER*)&size));
    verify(size > 0 && size <= 0xffffffff);

    auto buffer = (uint8_t*)memalloc(allocator, size);
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

static int64_t tick_frequency = 0;
int64_t get_current_timestamp() {
    if (!tick_frequency) {
        QueryPerformanceFrequency((LARGE_INTEGER*)&tick_frequency);
    }
    int64_t result = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&result);
    return result;
}

double timestamp_to_seconds(int64_t start, int64_t end) {
    return (end - start) / (double)tick_frequency;
}

wchar_t* get_skyrim_se_install_path() {
    wchar_t path[512];
    DWORD path_size = sizeof(path);

    auto result = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Bethesda Softworks\\Skyrim Special Edition",
        L"Installed Path",
        RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_SZ | RRF_SUBKEY_WOW6432KEY,
        nullptr,
        path,
        &path_size);
    auto err = GetLastError();
    verify(result == ERROR_SUCCESS);

    wchar_t* normalized_path = nullptr;;
    auto hr = PathAllocCanonicalize(path, 0, &normalized_path);
    verify(SUCCEEDED(hr));

    return normalized_path;
}

bool copy_file(const wchar_t* src, const wchar_t* dst) {
    return !!CopyFileW(src, dst, false);
}

void create_folder(const wchar_t* folder) {
    SHCreateDirectory(0, folder);
}

wchar_t* get_last_error() {
    wchar_t* err = nullptr;
    auto count = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, 0, GetLastError(), 0, (wchar_t*)&err, 1, nullptr);
    verify(count > 0);
    return err;
}

wchar_t* path_append(const wchar_t* a, const wchar_t* b) {
    wchar_t* out = nullptr;
    auto hr = PathAllocCombine(a, b, 0, &out);
    verify(SUCCEEDED(hr));
    return out;
}
