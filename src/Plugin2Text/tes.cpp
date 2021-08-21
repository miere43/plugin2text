#include "tes.hpp"
#include "common.hpp"
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.h"

bool Record::is_compressed() const {
    return (uint32_t)flags & (uint32_t)RecordFlags::Compressed;
}

uint8_t* Record::uncompress() const {
    verify(is_compressed());
    mz_ulong uncompressed_data_size = *(uint32_t*)((uint8_t*)this + sizeof(Record));
    uint8_t* compressed_data = (uint8_t*)this + sizeof(Record) + sizeof(uint32_t);

    auto uncompressed_data = new uint8_t[uncompressed_data_size];
    auto result = mz_uncompress(uncompressed_data, &uncompressed_data_size, compressed_data, data_size - sizeof(uint32_t));
    verify(result == MZ_OK);

    return (uint8_t*)uncompressed_data;
}
