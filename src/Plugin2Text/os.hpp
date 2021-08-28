#pragma once
#include <stdint.h>
#include "parseutils.hpp"

void* read_file(const wchar_t* path, uint32_t* size_out);
Slice allocate_virtual_memory(size_t size);
