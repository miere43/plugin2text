#pragma once
#include <stdint.h>

constexpr uint32_t fourcc(char const p[5]) {
    return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

enum class RecordType : uint32_t {
    GRUP = fourcc("GRUP"),
};

enum class RecordFlags : uint32_t {
    None = 0,
    TES4_Master = 0x1,
    Compressed = 0x40000,
};
inline RecordFlags operator|(const RecordFlags& a, const RecordFlags& b) { return (RecordFlags)((uint32_t)a | (uint32_t)b); }
inline RecordFlags& operator|=(RecordFlags& a, const RecordFlags& b) {
    return (a = a | b);
}

struct FormID {
    uint32_t value = 0;
};

struct Record {
    RecordType type = (RecordType)0;
    uint32_t data_size = 0;
    RecordFlags flags = RecordFlags::None;
    FormID id;
    uint16_t timestamp = 0;
    uint16_t version_control_info = 0;
    uint16_t version = 0;
    uint16_t unknown = 0;

    bool is_compressed() const;
    uint8_t* uncompress() const;
};
static_assert(sizeof(Record) == 24, "sizeof(Record) == 24");

enum class RecordGroupType : uint32_t {
    Top = 0,
    WorldChildren = 1,
    InteriorCellBlock = 2,
    InteriorCellSubBlock = 3,
    ExteriorCellBlock = 4,
    ExteriorCellSubBlock = 5,
    CellChildren = 6,
    TopicChildren = 7,
    CellPersistentChildren = 8,
    CellTemporaryChildren = 9,
};

RecordGroupType parse_record_group_type(const char* str, size_t count);
const char* record_group_type_to_string(RecordGroupType type);

struct GrupRecord {
    RecordType type = (RecordType)0;
    uint32_t group_size = 0;
    uint32_t label = 0;
    RecordGroupType group_type = (RecordGroupType)0;
    uint16_t timestamp = 0;
    uint16_t version_control_info = 0;
    uint32_t unknown = 0;
};
static_assert(sizeof(GrupRecord) == 24, "sizeof(GrupRecord) == 24");

enum class RecordFieldType : uint32_t {
};

#pragma pack(push, 1)
struct RecordField {
    RecordFieldType type = (RecordFieldType)0;
    uint16_t size = 0;
};
#pragma pack(pop)
static_assert(sizeof(RecordField) == 6, "sizeof(RecordField) == 6");
