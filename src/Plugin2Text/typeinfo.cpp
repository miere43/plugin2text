#include "typeinfo.hpp"
#include "common.hpp"
#include <stdlib.h>

Type Type_ZString{ TypeKind::ZString, "CString", 0 };
Type Type_LString{ TypeKind::LString, "LString", 0 };
Type Type_ByteArray{ TypeKind::ByteArray, "ByteArray", 0 };
Type Type_float{ TypeKind::Float, "float", sizeof(float) };
TypeInteger Type_int32{ "int32", sizeof(int), false };
TypeInteger Type_int16{ "int16", sizeof(int16_t), false };

TypeStructField Type_Vector3_Fields[] = {
    {
        &Type_float,
        "x",
    },
    {
        &Type_float,
        "y",
    },
    {
        &Type_float,
        "z",
    }
};

TypeStruct Type_Vector3{ "Vector3", 12, _countof(Type_Vector3_Fields), Type_Vector3_Fields };

const RecordFieldDef* RecordDef::get_field_def(RecordFieldType type) {
    for (int i = 0; i < field_count; ++i) {
        const auto& field = fields[i];
        if (field.type == type) {
            return &field;
        }
    }
    return nullptr;
}


TypeStructField Type_TES4_HEDR_Fields[] = {
    {
        &Type_float,
        "Version",
    },
    {
        &Type_int32,
        "Number Of Records",
    },
    {
        &Type_int32,
        "Next Object ID",
    },
};

TypeStruct Type_TES4_HEDR = {
    "TES4_HEDR",
    12,
    _countof(Type_TES4_HEDR_Fields),
    Type_TES4_HEDR_Fields,
};

RecordFieldDef Record_TES4_Fields[] = {
    {
        RecordFieldType::HEDR,
        &Type_TES4_HEDR,
        "Header",
    },
    {
        RecordFieldType::MAST,
        &Type_ZString,
        "Master File",
    },
    {
        RecordFieldType::CNAM,
        &Type_ZString,
        "Author",
    },
};

RecordDef Record_TES4{
    RecordType::TES4,
    "File Header",
    _countof(Record_TES4_Fields),
    Record_TES4_Fields,
};

TypeStructField Type_OBND_Fields[] = {
    {
        &Type_int16,
        "X1",
    },
    {
        &Type_int16,
        "Y1",
    },
    {
        &Type_int16,
        "Z1",
    },
        {
        &Type_int16,
        "X2",
    },
    {
        &Type_int16,
        "Y2",
    },
    {
        &Type_int16,
        "Z2",
    },
};

TypeStruct Type_OBND{ "Object Bounds", 12, _countof(Type_OBND_Fields), Type_OBND_Fields };

RecordFieldDef Record_WEAP_Fields[] = {
    {
        RecordFieldType::EDID,
        &Type_ZString,
        "Editor ID",
    },
    {
        RecordFieldType::OBND,
        &Type_OBND,
        "Object Bounds",
    },
    {
        RecordFieldType::FULL,
        &Type_LString,
        "Name",
    },
    {
        RecordFieldType::MODL,
        &Type_ZString,
        "Model File Name",
    },
};

RecordDef Record_WEAP{
    RecordType::WEAP,
    "Weapon",
    _countof(Record_WEAP_Fields),
    Record_WEAP_Fields,
};
