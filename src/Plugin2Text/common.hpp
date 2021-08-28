#pragma once
#include <stdint.h>

__declspec(noreturn) void verify_impl(const char* msg, const char* file, int line);

#ifdef _DEBUG
#define verify(cond) do { if (!(cond)) __debugbreak(); } while (0)
#else
#define verify(cond) do { if (!(cond)) verify_impl(#cond, __FILE__, __LINE__); } while (0)
#endif

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

#pragma pack(push, 1)
struct WString {
    uint16_t count;
    uint8_t data[1];
};
#pragma pack(pop)

struct BinaryReader {
    const uint8_t* start = nullptr;
    const uint8_t* now = nullptr;
    const uint8_t* end = nullptr;

    void read(void* out, size_t size);

    template<typename T>
    const T* advance() {
        return (T*)advance(sizeof(T));
    }

    template<typename T>
    T read() {
        return *(T*)advance(sizeof(T));
    }

    const void* advance(size_t size);
    const WString* advance_wstring();
};

template<typename T>
struct Array {
    T* data = nullptr;
    int count = 0;
    int capacity = 0;

    T& operator[](size_t index) {
        verify((int)index < count);
        return data[index];
    }

    const T& operator[](size_t index) const {
        verify((int)index < count);
        return data[index];
    }

    T* begin() { return data ? &data[0] : nullptr; }
    T* end() { return data ? &data[count] : nullptr; }
    const T* begin() const { return data ? &data[0] : nullptr; }
    const T* end() const { return data ? &data[count] : nullptr; }

    void push(const T& value);
    void free();
    int index_of(const T& value);
    void remove_at(int index);
    Array<T> clone() const;
};

template<typename T>
struct StaticArray {
    T* data = nullptr;
    size_t count = 0;
};
