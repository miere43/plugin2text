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

RecordGroupType parse_record_group_type(const char* str, size_t count) {
    #define CASE(m_type, m_string) if (strncmp(m_string, str, count) == 0) return RecordGroupType::m_type
    CASE(Top, "Top");
    CASE(WorldChildren, "World");
    CASE(InteriorCellBlock, "Interior Block");
    CASE(InteriorCellSubBlock, "Interior Sub-Block");
    CASE(ExteriorCellBlock, "Exterior");
    CASE(ExteriorCellSubBlock, "Exterior Sub-Block");
    CASE(CellChildren, "Cell");
    CASE(TopicChildren, "Topic");
    CASE(CellPersistentChildren, "Persistent");
    CASE(CellTemporaryChildren, "Temporary");
    #undef CASE
    verify(false);
    return (RecordGroupType)0;
}

const char* record_group_type_to_string(RecordGroupType type) {
    switch (type) {
        case RecordGroupType::Top:                    return "Top";
        case RecordGroupType::WorldChildren:          return "World";
        case RecordGroupType::InteriorCellBlock:      return "Interior Block";
        case RecordGroupType::InteriorCellSubBlock:   return "Interior Sub-Block";
        case RecordGroupType::ExteriorCellBlock:      return "Exterior";
        case RecordGroupType::ExteriorCellSubBlock:   return "Exterior Sub-Block";
        case RecordGroupType::CellChildren:           return "Cell";
        case RecordGroupType::TopicChildren:          return "Topic";
        case RecordGroupType::CellPersistentChildren: return "Persistent";
        case RecordGroupType::CellTemporaryChildren:  return "Temporary";
        default: verify(false); return nullptr;
    }
}
