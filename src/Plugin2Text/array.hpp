#include "common.hpp"
#include <string.h>

template<typename T>
void grow(Array<T>* arr, int increment) {
    int needed_count = arr->count + increment;
    if (needed_count > arr->capacity) {
        int new_capacity = arr->capacity < 4 ? 4 : arr->capacity * 2;
        if (new_capacity < needed_count) {
            new_capacity = needed_count;
        }
        T* new_data = memnew(*arr->allocator) T[new_capacity];
        memcpy(new_data, arr->data, sizeof(T) * arr->count);
        memdelete(*arr->allocator, arr->data);
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
}

template<typename T>
void Array<T>::push(const T& value) {
    grow(this, 1);
    data[count++] = value;
}

template<typename T>
void Array<T>::free() {
    memdelete(*allocator, data);
    data = nullptr;
    count = 0;
    capacity = 0;
}

template<typename T>
int Array<T>::index_of(const T& value) {
    for (int i = 0; i < count; ++i) {
        if (data[i] == value) {
            return i;
        }
    }
    return -1;
}

template<typename T>
void Array<T>::remove_at(int index) {
    verify(index >= 0 && index < count);
    memmove(&data[index], &data[index + 1], sizeof(T) * (size_t)(count - index));
    --count;
}

template<typename T>
Array<T> Array<T>::clone() const {
    Array<T> result;
    result.count = count;
    result.capacity = count;

    if (count) {
        result.data = new T[count];
        memcpy(result.data, data, sizeof(T) * count);
    }

    return result;
}
