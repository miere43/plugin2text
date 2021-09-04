#pragma once
#include "common.hpp"

extern "C" size_t strlen(const char* s);

bool string_equals(const String& a, const String& b);
bool string_equals(const char* a, size_t a_length, const char* b, size_t b_length);

inline bool operator==(const String& a, const String& b) { return string_equals(a, b); }
inline bool operator!=(const String& a, const String& b) { return !string_equals(a, b); }
inline bool operator==(const String& a, const char* b) { return string_equals(a.chars, a.count, b, b ? strlen(b) : 0); }
inline bool operator!=(const String& a, const char* b) { return !string_equals(a.chars, a.count, b, b ? strlen(b) : 0); }
