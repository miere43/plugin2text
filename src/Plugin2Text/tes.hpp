#pragma once
#include "common.hpp"

constexpr uint32_t fourcc(char const p[5]) {
    return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

enum class RecordType : uint32_t {
    TES4 = fourcc("TES4"),
    GRUP = fourcc("GRUP"),
    CELL = fourcc("CELL"),
    INFO = fourcc("INFO"),
    QUST = fourcc("QUST"),
    NPC_ = fourcc("NPC_"),
};

enum class RecordFlags : uint32_t {
    None = 0,
    TES4_Master = 0x1,
    TES4_Localized = 0x80,
    Compressed = 0x40000,
};
ENUM_BIT_OPS(uint32_t, RecordFlags);

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
};
static_assert(sizeof(RawRecord) == 24, "sizeof(Record) == 24");

struct RawRecordCompressed : RawRecord {
    uint32_t uncompressed_data_size;
};
static_assert(sizeof(RawRecordCompressed) == 28, "sizeof(RawRecordCompressed) == 28");

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
    DNAM = fourcc("DNAM"),
    VMAD = fourcc("VMAD"),
};

#pragma pack(push, 1)
struct RawRecordField {
    RecordFieldType type = (RecordFieldType)0;
    uint16_t size = 0;
};
static_assert(sizeof(RawRecordField) == 6, "sizeof(RecordField) == 6");

struct VMAD_Header {
    int16_t version;
    int16_t object_format;
    uint16_t script_count;
};

//struct PropertyObjectV1 {
//    FormID form_id;
//    int16_t alias;
//    uint16_t unused;
//};

struct VMAD_PropertyObjectV2 {
    uint16_t unused = 0;
    int16_t alias = 0;
    FormID form_id;
};
#pragma pack(pop)

const char* month_to_short_string(int month);
int short_string_to_month(const char* str);

enum class PapyrusPropertyType : uint8_t {
    None = 0,
    Object = 1,
    String = 2,
    Int = 3,
    Float = 4,
    Bool = 5,
    ObjectArray = 11,
    StringArray = 12,
    IntArray = 13,
    FloatArray = 14,
    BoolArray = 15,
};

enum class PapyrusFragmentFlags : uint8_t {
    None = 0,
    HasBeginScript = 0x1,
    HasEndScript = 0x2,
};
ENUM_BIT_OPS(uint8_t, PapyrusFragmentFlags);

struct VMAD_Field;

struct VMAD_Object {
    FormID form_id;
    int16_t alias = 0;
};

union VMAD_ScriptPropertyValue {
    VMAD_Object as_object;
    const WString* as_string;
    int as_int;
    float as_float;
    bool as_bool;
    struct {
        uint32_t count;
        VMAD_ScriptPropertyValue* values;
    } as_array;

    inline VMAD_ScriptPropertyValue() { }

    void parse(BinaryReader& r, const VMAD_Field* vmad, PapyrusPropertyType type);
};

struct VMAD_ScriptProperty {
    const WString* name = nullptr;
    uint8_t status = 0;
    PapyrusPropertyType type = PapyrusPropertyType::None;
    VMAD_ScriptPropertyValue value;

    void parse(BinaryReader& r, const VMAD_Field* vmad);
};

struct VMAD_Script {
    const WString* name = nullptr;
    uint8_t status = 0;

    Array<VMAD_ScriptProperty> properties;

    void parse(BinaryReader& r, const VMAD_Field* vmad, bool preserve_property_order);
};

struct VMAD_INFO_Fragment {
    const WString* script_name;
    const WString* fragment_name;

    void parse(BinaryReader& r);
};

struct VMAD_QUST_Fragment {
    uint16_t index;
    uint32_t log_entry;
    const WString* script_name;
    const WString* function_name;

    void parse(BinaryReader& r);
};

struct VMAD_QUST_Alias {
    VMAD_Object object;
    Array<VMAD_Script> scripts;
};

struct VMAD_Field {
    int16_t version = 0;
    int16_t object_format = 0;

    Array<VMAD_Script> scripts;
    bool contains_record_specific_info = false;

    union {
        struct {
            PapyrusFragmentFlags flags;
            const WString* script_name;
            VMAD_INFO_Fragment start_fragment;
            VMAD_INFO_Fragment end_fragment;
        } info;

        struct {
            const WString* file_name;
            Array<VMAD_QUST_Fragment> fragments;
            Array<VMAD_QUST_Alias> aliases;
        } qust;
    };

    inline VMAD_Field() { }
    
    void parse(const uint8_t* value, size_t size, RecordType record_type, bool preserve_property_order);
private:
    Array<VMAD_Script> parse_scripts(BinaryReader& r, uint16_t script_count, bool preserve_property_order);
};
