#pragma once
#include <stdint.h>
#include "parseutils.hpp"

StaticArray<uint8_t> read_file(const wchar_t* path);
Slice allocate_virtual_memory(size_t size);
void free_virtual_memory(Slice* slice);
void write_file(const wchar_t* path, const StaticArray<uint8_t>& data);
wchar_t* const* get_command_line_args(int* argc);
int64_t get_current_timestamp();
double timestamp_to_seconds(int64_t start, int64_t end);
wchar_t* get_skyrim_se_install_path();
bool copy_file(const wchar_t* src, const wchar_t* dst);
void create_folder(const wchar_t* folder);
wchar_t* get_last_error();
wchar_t* path_append(const wchar_t* a, const wchar_t* b);
