#pragma once
#include <stdint.h>
#include "tes.hpp"

enum class TypeKind {
    Unknown,
    Struct,
    Float,
    Integer,
    ByteArray,
    ByteArrayCompressed,
    ZString,
    LString,
    FormID,
    FormIDArray,
    Enum,
};

struct Type {
    TypeKind kind = TypeKind::Unknown;
    const char* name = nullptr;
    size_t size = 0;

    constexpr Type(TypeKind kind, const char* name, size_t size) : kind(kind), name(name), size(size) { }
};

struct TypeInteger : Type {
    bool is_unsigned = false;

    constexpr TypeInteger(const char* name, size_t size, bool is_unsigned) : Type(TypeKind::Integer, name, size), is_unsigned(is_unsigned) { }
};

struct TypeSpecial : Type {
};

struct TypeStructField {
    const Type* type = nullptr;
    const char* name = nullptr;
};

struct TypeStruct : Type {
    size_t field_count = 0;
    const TypeStructField* fields = nullptr;

    template<size_t N>
    constexpr TypeStruct(const char* name, size_t size, const TypeStructField(&fields)[N]) : Type(TypeKind::Struct, name, size), field_count(N), fields(fields) { }
};

struct TypeEnumField {
    const char* name = 0;
    uint32_t value = 0;
};

struct TypeEnum : Type {
    size_t field_count = 0;
    const TypeEnumField* fields = nullptr;

    template<size_t N>
    constexpr TypeEnum(const char* name, size_t size, const TypeEnumField(&fields)[N]) : Type(TypeKind::Enum, name, size), field_count(N), fields(fields) { }
};

extern Type Type_ZString;
extern Type Type_LString;
extern Type Type_ByteArray;
extern Type Type_ByteArrayCompressed;
extern Type Type_float;
extern Type Type_FormID;
extern TypeInteger Type_int8;
extern TypeInteger Type_int16;
extern TypeInteger Type_int32;
extern TypeInteger Type_int64;
extern TypeInteger Type_uint8;
extern TypeInteger Type_uint16;
extern TypeInteger Type_uint32;
extern TypeInteger Type_uint64;
extern TypeStruct Type_Vector3;

struct RecordFieldDef {
    RecordFieldType type;
    const Type* data_type;
    const char* comment = nullptr;

    constexpr RecordFieldDef(char const type[5], const Type* data_type, const char* comment) : type((RecordFieldType)fourcc(type)), data_type(data_type), comment(comment) { }
};

struct RecordDef {
    RecordType type;
    const char* comment = nullptr;
    size_t field_count = 0;
    const RecordFieldDef* fields = nullptr;

    template<size_t N>
    constexpr RecordDef(char const type[5], const char* comment, const RecordFieldDef(&fields)[N]) : type((RecordType)fourcc(type)), comment(comment), field_count(N), fields(fields) { }

    const RecordFieldDef* get_field_def(RecordFieldType type) const;
};

extern RecordDef Record_Common;

RecordDef* get_record_def(RecordType type);