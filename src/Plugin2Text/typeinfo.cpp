#include "typeinfo.hpp"
#include "common.hpp"
#include <stdlib.h>

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

const RecordFieldDef* RecordDef::get_field_def(RecordFieldType type) {
    for (int i = 0; i < field_count; ++i) {
        const auto& field = fields[i];
        if (field.type == type) {
            return &field;
        }
    }
    return nullptr;
}

static RecordFieldDef Record_Common_Fields[] = {
    rf_zstring("EDID", "Editor ID"),
    rf_lstring("FULL", "Name"),
};

static TypeStructField Type_TES4_HEDR_Fields[] = {
    sf_float("Version"),
    sf_int32("Number Of Records"),
    sf_int32("Next Object ID"),
};

TypeStruct Type_TES4_HEDR = {
    "TES4_HEDR",
    12,
    Type_TES4_HEDR_Fields,
};

static RecordFieldDef Record_TES4_Fields[] = {
    { "HEDR", &Type_TES4_HEDR, "Header" },
    rf_zstring("MAST", "Master File"),
    rf_zstring("CNAM", "Author"),
};

static TypeStructField Type_OBND_Fields[] = {
    sf_int16("X1"),
    sf_int16("Y1"),
    sf_int16("Z1"),
    sf_int16("X2"),
    sf_int16("Y2"),
    sf_int16("Z2"),
};

static TypeStruct Type_OBND{ "Object Bounds", 12, Type_OBND_Fields };

static TypeStructField Type_WEAP_DATA_Fields[] = {
    sf_int32("Value"),
    sf_float("Weight"),
    sf_int16("Damage"),
};

static TypeStruct Type_WEAP_DATA{ "Game Data", 10, Type_WEAP_DATA_Fields };

static TypeStructField Type_WEAP_DNAM_Fields[] = {
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
};

static TypeStruct Type_WEAP_DNAM{ "Data", 100, Type_WEAP_DNAM_Fields };

static TypeStructField Type_WEAP_CRDT_Fields[]{
    sf_uint16("Critical Damage"),
    sf_uint16("Unknown"),
    sf_float("Critical % Mult"),
    sf_uint32("Flags"),
    sf_uint32("Unknown"),
    sf_formid("Critical Spell Effect"),
    sf_uint32("Unknown"),
};

static TypeStruct Type_WEAP_CRDT{ "Critical Data", 24, Type_WEAP_CRDT_Fields };

static RecordFieldDef Record_WEAP_Fields[] = {
    { "OBND", &Type_OBND, "Object Bounds" },
    rf_zstring("MODL", "Model File Name"),
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
    { "DATA", &Type_WEAP_DATA, "Game Data" },
    { "DNAM", &Type_WEAP_DNAM, "Weapon Data" },
    { "CRDT", &Type_WEAP_CRDT, "Critical Data" },
    rf_int32("VNAM", "Detection Sound Level"),
};

static TypeStructField Type_QUST_DNAM_Fields[]{
    sf_uint8("Flags"),
    sf_uint8("Flags 2"),
    sf_uint8("Priority"),
    sf_uint8("Unknown"),
    sf_int32("Unknown 2"),
    sf_uint32("Quest Type"),
};

static TypeStruct Type_QUST_DNAM{ "Quest Data", 12, Type_QUST_DNAM_Fields };

static RecordFieldDef Record_QUST_Fields[] = {
    { "DNAM", &Type_QUST_DNAM, "Quest Data" },
};

RecordDef Record_Common{ "0000", "-- common -- ", Record_Common_Fields};
RecordDef Record_TES4 { "TES4", "File Header", Record_TES4_Fields };
RecordDef Record_WEAP { "WEAP", "Weapon", Record_WEAP_Fields };
RecordDef Record_QUST{ "QUST", "Quest", Record_QUST_Fields };