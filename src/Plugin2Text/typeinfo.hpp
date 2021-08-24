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
    ByteArrayFixed,
    ZString,
    LString,
    FormID,
    FormIDArray,
    Enum,
    Boolean, // 1 byte,
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
    uint32_t value = 0;
    const char* name = 0;
};

struct TypeEnum : Type {
    size_t field_count = 0;
    const TypeEnumField* fields = nullptr;
    bool flags = false;

    template<size_t N>
    constexpr TypeEnum(const char* name, size_t size, const TypeEnumField(&fields)[N], bool flags) : Type(TypeKind::Enum, name, size), field_count(N), fields(fields), flags(flags) { }
};

extern Type Type_ZString;
extern Type Type_LString;
extern Type Type_ByteArray;
extern Type Type_ByteArrayCompressed;
extern Type Type_float;
extern Type Type_FormID;
extern Type Type_FormIDArray;
extern Type Type_bool;
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

struct RecordFlagDef {
    uint32_t bit = 0;
    const char* name = nullptr;
};

template<typename T>
struct StaticArray {
    T* data = nullptr;
    size_t count = 0;
};

struct RecordDef {
    RecordType type;
    const char* comment = nullptr;
    StaticArray<RecordFieldDef> fields;
    StaticArray<RecordFlagDef> flags;

    constexpr RecordDef(
            char const type[5],
            const char* comment,
            StaticArray<RecordFieldDef> fields)
        : type((RecordType)fourcc(type))
        , comment(comment)
        , fields(fields)
    { }

    constexpr RecordDef(
            char const type[5],
            const char* comment,
            StaticArray<RecordFieldDef> fields,
            StaticArray<RecordFlagDef> flags)
        : type((RecordType)fourcc(type))
        , comment(comment)
        , fields(fields)
        , flags(flags)
    { }

    const RecordFieldDef* get_field_def(RecordFieldType type) const;
};

extern RecordDef Record_Common;

RecordDef* get_record_def(RecordType type);