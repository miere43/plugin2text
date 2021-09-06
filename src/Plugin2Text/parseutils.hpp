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

    template<size_t N>
    void write_literal(const char(&data)[N]) {
        write_bytes(data, N - 1);
    }

    template<typename T>
    void write_value(const T& data) {
        write_bytes(&data, sizeof(T));
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
                write_value((uint8_t)value);
            } break;

            case sizeof(uint16_t): {
                write_value((uint16_t)value);
            } break;

            case sizeof(uint32_t): {
                write_value((uint32_t)value);
            } break;

            case sizeof(uint64_t): {
                write_value(value);
            } break;

            default: {
                verify(false);
            } break;
        }
    }

    void write_string(const String& value) {
        write_bytes(value.chars, value.count);
    }

    size_t size() const {
        return now - start;
    }
};