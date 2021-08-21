#pragma once
#include <stdint.h>

constexpr uint32_t fourcc(char const p[5]) {
    return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

enum class RecordType : uint32_t {
    GRUP = fourcc("GRUP"),
};

enum class RecordFlags : uint32_t {
    TES4_Master = 0x1,
    Compressed = 0x40000,
};

struct Record {
    RecordType type;
    uint32_t data_size;
    RecordFlags flags;
    uint32_t id;
    uint32_t unused0;
    uint32_t unused1;

    bool is_compressed() const;
    uint8_t* uncompress() const;
};
static_assert(sizeof(Record) == 24, "sizeof(Record) == 24");

struct GrupRecord {
    RecordType type;
    uint32_t group_size;
    union {
        uint32_t label_as_uint32;
        char     label_as_chars[4];
    };
    uint32_t group_type;
    uint32_t unused0;
    uint32_t unused1;
};
static_assert(sizeof(GrupRecord) == 24, "sizeof(GrupRecord) == 24");

enum class RecordFieldType : uint32_t {
};

#pragma pack(push, 1)
struct RecordField {
    RecordFieldType type;
    uint16_t size;
};
#pragma pack(pop)
static_assert(sizeof(RecordField) == 6, "sizeof(RecordField) == 6");
