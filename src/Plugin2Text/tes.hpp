#pragma once
#include <stdint.h>

constexpr uint32_t fourcc(char const p[5]) {
    return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

enum class RecordType : uint32_t {
    TES4 = 0x34534554,
    WEAP = 0x50414557,
    GRUP = 0x50555247,
    ARMO = 0x4F4D5241,
    HEDR = 0x52444548,
    CELL = 0x4c4c4543,
    QUST = fourcc("QUST"),
    REFR = fourcc("REFR"),
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
    FULL = 1280070982,
    BOD2 = 0x32444f42,
    BODT = 0x54444f42,
    BMDT = 0x54444d42,
    DNAM = 0x4d414e44,
    KSIZ = 0x5a49534b,
    KWDA = 0x4144574b,
    MAST = 0x5453414d,
    CNAM = fourcc("CNAM"),
    INTV = fourcc("INTV"),
    HEDR = fourcc("HEDR"),
    EDID = fourcc("EDID"),
    OBND = fourcc("OBND"),
    MODL = fourcc("MODL"),
};

#pragma pack(push, 1)
struct RecordField {
    RecordFieldType type;
    uint16_t size;
};
#pragma pack(pop)
static_assert(sizeof(RecordField) == 6, "sizeof(RecordField) == 6");
