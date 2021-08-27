#pragma once
#include <stdint.h>
#include "tes.hpp"
#include "common.hpp"

enum class TypeKind {
    Unknown,
    Struct,
    Float,
    Integer,
    ByteArray,
    ByteArrayCompressed,
    ByteArrayFixed,
    ByteArrayRLE,
    ZString,
    LString,
    WString,
    FormID,
    FormIDArray,
    Enum,
    Boolean, // 1 byte,
    VMAD,
    Constant,
    Filter,
    Vector3,
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

struct TypeConstant : Type {
    const uint8_t* bytes = nullptr;

    constexpr TypeConstant(const char* name, size_t size, const uint8_t* bytes) : Type(TypeKind::Constant, name, size), bytes(bytes) { }
};

struct TypeFilter : Type {
    const Type* inner_type;
    void (*preprocess)(void* data, size_t size);
    
    constexpr TypeFilter(
            const Type* inner_type,
            void (*preprocess)(void* data, size_t size))
        : Type(TypeKind::Filter, inner_type->name, inner_type->size)
        , inner_type(inner_type)
        , preprocess(preprocess)
    { }
};

struct Vector3 {
    float x = 0;
    float y = 0;
    float z = 0;
};

constexpr bool VMAD_use_byte_array = true; // @TODO

extern Type Type_ZString;
extern Type Type_LString;
extern Type Type_WString;
extern Type Type_ByteArray;
extern Type Type_ByteArrayCompressed;
extern Type Type_ByteArrayRLE;
extern Type Type_float;
extern Type Type_FormID;
extern Type Type_FormIDArray;
extern Type Type_bool;
extern Type Type_VMAD;
extern TypeInteger Type_int8_t;
extern TypeInteger Type_int16_t;
extern TypeInteger Type_int32_t;
extern TypeInteger Type_int64_t;
extern TypeInteger Type_uint8_t;
extern TypeInteger Type_uint16_t;
extern TypeInteger Type_uint32_t;
extern TypeInteger Type_uint64_t;
extern Type Type_Vector3;

enum class PapyrusPropertyType : uint8_t {
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
extern TypeEnum Type_PapyrusPropertyType;

template<typename T>
inline const Type* resolve_type() {
    static_assert(false, "unknown type");
}

#define RESOLVE_TYPE(x) template<> inline const Type* resolve_type<x>() { return &Type_##x; }
RESOLVE_TYPE(int8_t);
RESOLVE_TYPE(int16_t);
RESOLVE_TYPE(int32_t);
RESOLVE_TYPE(int64_t);
RESOLVE_TYPE(uint8_t);
RESOLVE_TYPE(uint16_t);
RESOLVE_TYPE(uint32_t);
RESOLVE_TYPE(uint64_t);
RESOLVE_TYPE(PapyrusPropertyType);
RESOLVE_TYPE(FormID);
RESOLVE_TYPE(WString);

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

constexpr char ByteArrayRLE_StreamStart = '!';
constexpr size_t ByteArrayRLE_MaxStreamValue = '~' - ByteArrayRLE_StreamStart;
constexpr char ByteArrayRLE_SequenceMarker_00 = '?';
constexpr char ByteArrayRLE_SequenceMarker_FF = '!';
