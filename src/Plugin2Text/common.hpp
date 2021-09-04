#pragma once
#include <stdint.h>

enum class MemoryOperation {
    Allocate,
    Free,
};

struct Allocator {
    void* (*exec)(Allocator& self, MemoryOperation op, void* ptr, size_t size);
};

struct LinearAllocator : Allocator {
    uint8_t* start = nullptr;
    uint8_t* now = nullptr;
    const uint8_t* end = nullptr;

    inline size_t remaining_size() const {
        return end - now;
    }
};

extern Allocator stdalloc;
extern LinearAllocator tmpalloc;

void* operator new(size_t size, Allocator& allocator);
void* operator new[](size_t size, Allocator& allocator);
void operator delete(void* block, Allocator& allocator);
void operator delete[](void* block, Allocator& allocator);

#define memnew(allocator) new(allocator)
#define memalloc(allocator, size) ::operator new(size, allocator)
#define memdelete(allocator, block) ::operator delete(block, allocator)
#define stdnew new(stdalloc)
#define tmpnew new(tmpalloc)
#define stddelete(block) ::operator delete(block, stdalloc)

__declspec(noreturn) void verify_impl(const char* msg, const char* file, int line);

#define verify(cond) do { if (!(cond)) { verify_impl(#cond, __FILE__, __LINE__); } } while (0)

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
bool string_starts_with(const wchar_t* a, const wchar_t* b);
int string_index_of(const wchar_t* str, wchar_t c);
wchar_t* substring(Allocator& allocator, const wchar_t* start, const wchar_t* end);
wchar_t* string_replace_extension(Allocator& allocator, const wchar_t* path, const wchar_t* new_extension);

#pragma pack(push, 1)
struct WString {
    uint16_t count;
    char data[1];
};
#pragma pack(pop)

struct BinaryReader {
    const uint8_t* start = nullptr;
    const uint8_t* now = nullptr;
    const uint8_t* end = nullptr;

    inline BinaryReader() { }
    inline BinaryReader(const uint8_t* data, size_t size) : start(data), now(data), end(data + size) { }

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
    Allocator* allocator = nullptr;

    inline Array() : data(nullptr), count(0), capacity(0), allocator(&stdalloc) { }
    inline Array(Allocator& allocator) : data(nullptr), count(0), capacity(0), allocator(&allocator) { }

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

    T* begin() { return data ? &data[0] : nullptr; }
    T* end() { return data ? &data[count] : nullptr; }
    const T* begin() const { return data ? &data[0] : nullptr; }
    const T* end() const { return data ? &data[count] : nullptr; }
};

#define ENUM_BIT_OPS(m_decltype, m_enum) \
    inline m_enum operator&(const m_enum& a, const m_enum& b) { return (m_enum)((m_decltype)a & (m_decltype)b); } \
    inline m_enum operator|(const m_enum& a, const m_enum& b) { return (m_enum)((m_decltype)a | (m_decltype)b); } \
    inline m_enum& operator|=(m_enum& a, const m_enum& b) { return (a = a | b); }

template<typename T>
inline bool is_bit_set(const T& obj, const T& bit) {
    return (obj & bit) != (T)0;
}

// @TODO: split into EspParserOptions, TextToEspOptions, etc.
enum class ProgramOptions : uint32_t {
    None = 0,
    ExportTimestamp = 0x1,
    PreserveRecordOrder = 0x2, // @TODO: Rename to PreserveOrder
    PreserveJunk = 0x4,
    DebugZLib = 0x8,
    ExportRelatedFiles = 0x10,
};
ENUM_BIT_OPS(uint32_t, ProgramOptions);

struct TempScope {
    uint8_t* scope_start;
    
    inline TempScope() {
        scope_start = tmpalloc.now;
    }

    inline ~TempScope() {
        tmpalloc.now = scope_start;
    }
};

#define TEMP_SCOPE() auto _tmpscope = TempScope()