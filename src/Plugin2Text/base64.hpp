#pragma once
#include <stdint.h>

size_t base64_encode(const uint8_t* bytes_to_encode, size_t in_len, char* out_buffer, size_t out_buffer_size);
size_t base64_decode(const char* encoded_string, size_t encoded_string_count, uint8_t * out_buffer, size_t out_buffer_size);
