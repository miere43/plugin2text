#pragma once
#include <stdint.h>
#include "parseutils.hpp"

StaticArray<uint8_t> read_file(const wchar_t* path);
Slice allocate_virtual_memory(size_t size);
void free_virtual_memory(Slice* slice);
void write_file(const wchar_t* path, const StaticArray<uint8_t>& data);
wchar_t* const* get_command_line_args(int* argc);

