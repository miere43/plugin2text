#include "typeinfo.hpp"
#include "common.hpp"
#include <stdlib.h>

constexpr RecordFieldDef rf_zstring(char const type[5], const char* name) {
    return { (RecordFieldType)fourcc(type), &Type_ZString, name };
}

constexpr RecordFieldDef rf_lstring(char const type[5], const char* name) {
    return { (RecordFieldType)fourcc(type), &Type_LString, name };
}

constexpr RecordFieldDef rf_float(char const type[5], const char* name) {
    return { (RecordFieldType)fourcc(type), &Type_float, name };
}

constexpr RecordFieldDef rf_int32(char const type[5], const char* name) {
    return { (RecordFieldType)fourcc(type), &Type_int32, name };
}

constexpr TypeStructField sf_int16(const char* name) {
    return { &Type_int16, name };
}

constexpr TypeStructField sf_int32(const char* name) {
    return { &Type_int32, name };
}

constexpr TypeStructField sf_float(const char* name) {
    return { &Type_float, name };
}

Type Type_ZString{ TypeKind::ZString, "CString", 0 };
Type Type_LString{ TypeKind::LString, "LString", 0 };
Type Type_ByteArray{ TypeKind::ByteArray, "Byte Array", 0 };
Type Type_float{ TypeKind::Float, "float", sizeof(float) };
Type Type_FormID{ TypeKind::FormID, "Form ID", sizeof(int) };
TypeInteger Type_int32{ "int32", sizeof(int), false };
TypeInteger Type_int16{ "int16", sizeof(int16_t), false };

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
    {
        RecordFieldType::HEDR,
        &Type_TES4_HEDR,
        "Header",
    },
    rf_zstring("MAST", "Master File"),
    rf_zstring("CNAM", "Author"),
};

TypeStructField Type_OBND_Fields[] = {
    sf_int16("X1"),
    sf_int16("Y1"),
    sf_int16("Z1"),
    sf_int16("X2"),
    sf_int16("Y2"),
    sf_int16("Z2"),
};

TypeStruct Type_OBND{ "Object Bounds", 12, Type_OBND_Fields };

RecordFieldDef Record_WEAP_Fields[] = {
    rf_zstring("EDID", "Editor ID"),
    { RecordFieldType::OBND, &Type_OBND, "Object Bounds" },
    rf_lstring("FULL", "Name"),
    rf_zstring("MODL", "Model File Name"),
};

RecordDef Record_TES4 { RecordType::TES4, "File Header", Record_TES4_Fields };
RecordDef Record_WEAP { RecordType::WEAP, "Weapon", Record_WEAP_Fields };
