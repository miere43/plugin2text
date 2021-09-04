#include "string.hpp"
#include <string.h>

bool string_equals(const String& a, const String& b) {
    return string_equals(a.chars, a.count, b.chars, b.count);
}

bool string_equals(const char* a, size_t a_length, const char* b, size_t b_length) {
    if (a_length != b_length) return false;
    if (a == b) return true;
    return strncmp(a, b, a_length) == 0;
}
