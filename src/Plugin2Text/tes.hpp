#pragma once
#include <stdint.h>

constexpr uint32_t fourcc(char const p[5]) {
    return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

enum class RecordType : uint32_t {
    GRUP = fourcc("GRUP"),
    CELL = fourcc("CELL"),
    INFO = fourcc("INFO"),
    QUST = fourcc("QUST"),
};

enum class RecordFlags : uint32_t {
    None = 0,
    TES4_Localized = 0x80,
    Compressed = 0x40000,
};

inline RecordFlags operator&(const RecordFlags& a, const RecordFlags& b) { return (RecordFlags)((uint32_t)a | (uint32_t)b); }
inline RecordFlags operator|(const RecordFlags& a, const RecordFlags& b) { return (RecordFlags)((uint32_t)a | (uint32_t)b); }
inline RecordFlags& operator|=(RecordFlags& a, const RecordFlags& b) {
    return (a = a | b);
}

template<typename T>
inline T clear_bit(const T& flags, const T& bit) {
    static_assert(sizeof(T) == 4, "invalid size");
    return (T)((uint32_t)flags & (~(uint32_t)bit));
}

struct FormID {
    uint32_t value = 0;
};

struct RawRecord {
    RecordType type = (RecordType)0;
    uint32_t data_size = 0;
    RecordFlags flags = RecordFlags::None;
    FormID id;
    uint16_t timestamp = 0;
    uint8_t last_user_id = 0;
    uint8_t current_user_id = 0;
    uint16_t version = 0;
    uint16_t unknown = 0;

    bool is_compressed() const;
    uint8_t* uncompress(uint32_t* out_uncompressed_data_size) const;
};
static_assert(sizeof(RawRecord) == 24, "sizeof(Record) == 24");

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

const char* record_group_type_to_string(RecordGroupType type);

struct RawGrupRecord {
    RecordType type = (RecordType)0;
    uint32_t group_size = 0;
    union {
        struct {
            int16_t grid_y;
            int16_t grid_x;
        };
        uint32_t label;
    };
    RecordGroupType group_type = (RecordGroupType)0;
    uint16_t timestamp = 0;
    uint8_t last_user_id = 0;
    uint8_t current_user_id = 0;
    uint32_t unknown = 0;

    inline RawGrupRecord() : label(0) { }
};
static_assert(sizeof(RawGrupRecord) == 24, "sizeof(GrupRecord) == 24");

enum class RecordFieldType : uint32_t {
};

#pragma pack(push, 1)
struct RawRecordField {
    RecordFieldType type = (RecordFieldType)0;
    uint16_t size = 0;
};
#pragma pack(pop)
static_assert(sizeof(RawRecordField) == 6, "sizeof(RecordField) == 6");

const char* month_to_short_string(int month);
int short_string_to_month(const char* str);
