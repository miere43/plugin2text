#pragma once
#include <stdint.h>

#define verify(cond) do { if (!(cond)) __debugbreak(); } while (0)

template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

__declspec(noreturn) void exit_error(const wchar_t* format, ...);
bool string_equals(const wchar_t* a, const wchar_t* b);
bool memory_equals(const void* a, const void* b, size_t size);
int string_last_index_of(const wchar_t* str, char c);