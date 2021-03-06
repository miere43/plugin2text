#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj_core.h>
#include "common.hpp"
#include "os.hpp"
#include <PathCch.h>

#pragma comment(lib, "pathcch.lib")

StaticArray<uint8_t> try_read_file(Allocator& allocator, const wchar_t* path) {
    StaticArray<uint8_t> result;
    
    auto handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        return result;
    }
    defer(CloseHandle(handle));

    uint64_t size = 0;
    if (!GetFileSizeEx(handle, (LARGE_INTEGER*)&size)) {
        return result;
    } else if (size <= 0) {
        return result;
    } else if (size > 0xffffffff) {
        SetLastError(ERROR_FILE_TOO_LARGE);
        return result;
    }

    auto buffer = (uint8_t*)memalloc(allocator, size);
    DWORD read = 0;
    if (!ReadFile(handle, buffer, (uint32_t)size, &read, nullptr) || read != size) {
        memdelete(allocator, buffer);
        return result;
    }

    result = { buffer, read };
    return result;
}

StaticArray<uint8_t> read_file(Allocator& allocator, const wchar_t* path) {
    auto result = try_read_file(allocator, path);
    verify(result.count);
    return result;
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
    auto handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
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
    wchar_t path[512]{ 0 };
    DWORD path_size = sizeof(path);

    auto result = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Bethesda Softworks\\Skyrim Special Edition",
        L"Installed Path",
        RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_SZ | RRF_SUBKEY_WOW6432KEY,
        nullptr,
        path,
        &path_size);
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

void Path::append(const wchar_t* append_path) {
    HRESULT hr = PathCchAppend(path, _countof(path), append_path);
    verify(SUCCEEDED(hr));
}

void Path::append(std::initializer_list<const wchar_t*> append_paths) {
    for (const auto append_path : append_paths) {
        HRESULT hr = PathCchAppend(path, _countof(path), append_path);
        verify(SUCCEEDED(hr));
    }
}

const wchar_t* get_current_directory() {
    auto count = GetCurrentDirectoryW((DWORD)tmpalloc.remaining_size(), (wchar_t*)tmpalloc.now);
    verify(GetLastError() == NO_ERROR);
    return (wchar_t*)memalloc(tmpalloc, count * sizeof(wchar_t));
}