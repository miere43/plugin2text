#include "esplugin.hpp"
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
        0,
    },
    {
        &Type_float,
        "y",
        4,
    },
    {
        &Type_float,
        "z",
        8,
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
