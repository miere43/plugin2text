#include "typeinfo.hpp"
#include "common.hpp"
#include <stdlib.h>

#define CONCAT(a, b) a##b

constexpr RecordFieldDef rf_zstring(char const type[5], const char* name) {
    return { type, &Type_ZString, name };
}

constexpr RecordFieldDef rf_lstring(char const type[5], const char* name) {
    return { type, &Type_LString, name };
}

constexpr RecordFieldDef rf_float(char const type[5], const char* name) {
    return { type, &Type_float, name };
}

constexpr RecordFieldDef rf_int32(char const type[5], const char* name) {
    return { type, &Type_int32, name };
}

constexpr RecordFieldDef rf_formid(char const type[5], const char* name) {
    return { type, &Type_FormID, name };
}

#define rf_subrecord(m_record, m_subrecord, m_name) \
    { #m_subrecord, &CONCAT(Type_, m_record)_##m_subrecord, m_name }

constexpr TypeStructField sf_int8(const char* name) {
    return { &Type_int8, name };
}

constexpr TypeStructField sf_int16(const char* name) {
    return { &Type_int16, name };
}

constexpr TypeStructField sf_int32(const char* name) {
    return { &Type_int32, name };
}

constexpr TypeStructField sf_int64(const char* name) {
    return { &Type_int64, name };
}

constexpr TypeStructField sf_uint8(const char* name) {
    return { &Type_uint8, name };
}

constexpr TypeStructField sf_uint16(const char* name) {
    return { &Type_uint16, name };
}

constexpr TypeStructField sf_uint32(const char* name) {
    return { &Type_uint32, name };
}

constexpr TypeStructField sf_uint64(const char* name) {
    return { &Type_uint64, name };
}

constexpr TypeStructField sf_float(const char* name) {
    return { &Type_float, name };
}

constexpr TypeStructField sf_formid(const char* name) {
    return { &Type_FormID, name };
}

Type Type_ZString{ TypeKind::ZString, "CString", 0 };
Type Type_LString{ TypeKind::LString, "LString", 0 };
Type Type_ByteArray{ TypeKind::ByteArray, "Byte Array", 0 };
Type Type_float{ TypeKind::Float, "float", sizeof(float) };
Type Type_FormID{ TypeKind::FormID, "Form ID", sizeof(int) };
Type Type_FormIDArray{ TypeKind::FormIDArray, "Form ID Array", 0 };
TypeInteger Type_int8{ "int8", sizeof(int8_t), false };
TypeInteger Type_int16{ "int16", sizeof(int16_t), false };
TypeInteger Type_int32{ "int32", sizeof(int32_t), false };
TypeInteger Type_int64{ "int64", sizeof(int64_t), false };
TypeInteger Type_uint8{ "uint8", sizeof(uint8_t), true };
TypeInteger Type_uint16{ "uint16", sizeof(uint16_t), true };
TypeInteger Type_uint32{ "uint32", sizeof(uint32_t), true };
TypeInteger Type_uint64{ "uint64", sizeof(uint64_t), true };

TypeStructField Type_Vector3_Fields[] = {
    sf_float("X"),
    sf_float("Y"),
    sf_float("Z"),
};

TypeStruct Type_Vector3{ "Vector3", 12, Type_Vector3_Fields };

const RecordFieldDef* RecordDef::get_field_def(RecordFieldType type) const {
    for (int i = 0; i < field_count; ++i) {
        const auto& field = fields[i];
        if (field.type == type) {
            return &field;
        }
    }
    return nullptr;
}

#define TYPE_STRUCT(m_type, m_name, m_size, ...)           \
    static TypeStructField CONCAT(Type_, m_type)_Fields[]{ \
        __VA_ARGS__                                        \
    };                                                     \
                                                           \
    static TypeStruct Type_##m_type{                       \
        m_name,                                            \
        m_size,                                            \
        CONCAT(Type_, m_type)_Fields                       \
    }

TYPE_STRUCT(CELL_XCLL, "Lighting", 92,
    sf_uint32("Ambient"),
    sf_uint32("Directional"),
    sf_uint32("Fog Near"),
    sf_float("Fog Near"),
    sf_float("Fog Far"),
    sf_int32("Rotation XY"),
    sf_int32("Rotation Z"),
);

TYPE_STRUCT(QUST_DNAM, "Quest Data", 12,
    sf_uint8("Flags"),
    sf_uint8("Flags 2"),
    sf_uint8("Priority"),
    sf_uint8("Unknown"),
    sf_int32("Unknown 2"),
    sf_uint32("Quest Type"),
);

TYPE_STRUCT(WEAP_CRDT, "Critical Data", 24,
    sf_uint16("Critical Damage"),
    sf_uint16("Unknown"),
    sf_float("Critical % Mult"),
    sf_uint32("Flags"),
    sf_uint32("Unknown"),
    sf_formid("Critical Spell Effect"),
    sf_uint32("Unknown"),
);

TYPE_STRUCT(WEAP_DNAM, "Data", 100,
    sf_uint8("Animation Type"),
    sf_int8("Unknown 0"),
    sf_int16("Unknown 1"),
    sf_float("Speed"),
    sf_float("Reach"),
    sf_uint16("Flags"),
    sf_uint16("Flags?"),
    sf_float("Sight FOV"),
    sf_uint32("Blank"),
    sf_uint8("VATS to hit"),
    sf_int8("Unknown 1"),
    sf_uint8("Projectiles"),
    sf_int8("Embedded Weapon"),
    sf_float("Min Range"),
    sf_float("Max Range"),
    sf_uint32("Unknown 2"),
    sf_uint32("Flags"),
    sf_float("Unknown"),
    sf_float("Unknown"),
    sf_float("Rumble Left"),
    sf_float("Rumble Right"),
    sf_float("Rumble Duration"),
    sf_uint32("Blank"),
    sf_uint32("Blank"),
    sf_uint32("Blank"),
    sf_int32("Skill"),
    sf_uint32("Blank"),
    sf_uint32("Blank"),
    sf_int32("Resist"),
    sf_uint32("Blank"),
    sf_float("Stagger"),
);

TYPE_STRUCT(WEAP_DATA, "Game Data", 10,
    sf_int32("Value"),
    sf_float("Weight"),
    sf_int16("Damage"),
);

TYPE_STRUCT(TES4_HEDR, "Header", 12,
    sf_float("Version"),
    sf_int32("Number Of Records"),
    sf_int32("Next Object ID"),
);

TYPE_STRUCT(OBND, "Object Bounds", 12,
    sf_int16("X1"),
    sf_int16("Y1"),
    sf_int16("Z1"),
    sf_int16("X2"),
    sf_int16("Y2"),
    sf_int16("Z2"),
);

TYPE_STRUCT(CONT_CNTO, "Item", 8,
    sf_formid("Item"),
    sf_uint32("Count"),
);

//static RecordFieldDef Record_CELL_Fields[]{
    //{ "XCLL", &Type_CELL_XCLL, "Lighting" },
//};

#define RECORD(m_type, m_name, ...)                         \
    static RecordFieldDef CONCAT(Record_, m_type)_Fields[]{ \
        __VA_ARGS__                                         \
    };                                                      \
                                                            \
    static RecordDef Record_##m_type{                       \
        #m_type,                                            \
        m_name,                                             \
        CONCAT(Record_, m_type)_Fields,                     \
    }

static RecordFieldDef Record_Common_Fields[] = {
    rf_zstring("EDID", "Editor ID"),
    rf_lstring("FULL", "Name"),
    { "OBND", &Type_OBND, "Object Bounds" },
    rf_zstring("MODL", "Model File Name"),
};

RecordDef Record_Common{ "0000", "-- common -- ", Record_Common_Fields};

RECORD(TES4, "File Header",
    rf_subrecord(TES4, HEDR, "Header"),
    rf_zstring("MAST", "Master File"),
    rf_zstring("CNAM", "Author"),
);

RECORD(WEAP, "Weapon",
    rf_formid("ETYP", "Equipment Type"),
    rf_formid("BIDS", "Block Bash Impact Data Set"),
    rf_formid("BAMT", "Alternate Block Material"),
    rf_int32("KSIZ", "Keyword Count"),
    { "KWDA", &Type_FormIDArray, "Keywords" },
    rf_lstring("DESC", "Description"),
    rf_formid("INAM", "Impact Data Set"),
    rf_formid("WNAM", "1st Person Model Object"),
    rf_formid("TNAM", "Attack Fail Sound"),
    rf_formid("NAM9", "Equip Sound"),
    rf_formid("NAM8", "Unequip Sound"),
    rf_subrecord(WEAP, DATA, "Game Data"),
    rf_subrecord(WEAP, DNAM, "Weapon Data"),
    rf_subrecord(WEAP, CRDT, "Critical Data"),
    rf_int32("VNAM", "Detection Sound Level"),
);

RECORD(QUST, "Quest",
    { "DNAM", &Type_QUST_DNAM, "Quest Data" },
);

RecordDef Record_CELL{ "CELL", "Cell", Record_Common_Fields }; // @TODO

RECORD(REFR, "Reference",
    rf_formid("NAME", "Form ID"),
);

RECORD(CONT, "Container",
    rf_int32("COCT", "Item Count"),
    rf_subrecord(CONT, CNTO, "Items"),
);

#undef RECORD

RecordDef* get_record_def(RecordType type) {
    #define CASE(rec) case (RecordType)fourcc(#rec): return &Record_##rec
    switch (type) {
        CASE(TES4);
        CASE(WEAP);
        CASE(QUST);
        CASE(CELL);
        CASE(REFR);
        CASE(CONT);
    }
    #undef CASE
    return nullptr;
}