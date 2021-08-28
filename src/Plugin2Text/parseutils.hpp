#pragma once
#include "common.hpp"
#include <string.h>

struct Slice {
    uint8_t* start = nullptr;
    uint8_t* now = nullptr;
    const uint8_t* end = nullptr;

    inline size_t remaining_size() const {
        return end - now;
    }

    template<typename T>
    T* advance() {
        T* obj = (T*)advance(sizeof(T));
        *obj = T();
        return obj;
    }

    inline uint8_t* advance(size_t count) {
        verify(now + count <= end);
        auto result = now;
        now += count;
        return result;
    }

    template<typename T>
    void write_constant(const T& value) {
        write_bytes(&value, sizeof(T));
    }

    template<typename T>
    void write_struct(const T* data) {
        write_bytes(data, sizeof(T));
    }

    template<typename T>
    void write_struct_at(uint8_t* pos, const T* data) {
        write_bytes_at(pos, data, sizeof(T));
    }

    void write_bytes(const void* data, size_t size) {
        write_bytes_at(now, data, size);
        now += size;
    }

    void write_bytes_at(uint8_t* pos, const void* data, size_t size) {
        verify(pos >= start);
        verify(pos + size <= end);
        memcpy(pos, data, size);
    }

    void write_integer_of_size(uint64_t value, size_t size) {
        switch (size) {
            case sizeof(uint8_t): {
                const auto value8 = (uint8_t)value;
                write_struct(&value8);
            } break;

            case sizeof(uint16_t): {
                const auto value16 = (uint16_t)value;
                write_struct(&value16);
            } break;

            case sizeof(uint32_t): {
                const auto value32 = (uint32_t)value;
                write_struct(&value32);
            } break;

            case sizeof(uint64_t): {
                write_struct(&value);
            } break;

            default: {
                verify(false);
            } break;
        }
    }

    size_t size() const {
        return now - start;
    }
};