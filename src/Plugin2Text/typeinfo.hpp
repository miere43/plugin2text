#pragma once
#include <stdint.h>

enum class RecordType : uint32_t {
    TES4 = 0x34534554,
    WEAP = 0x50414557,
    GRUP = 0x50555247,
    ARMO = 0x4F4D5241,
    HEDR = 0x52444548,
    CELL = 0x4c4c4543,
};

struct Record {
    RecordType type;
    uint32_t data_size;
    uint32_t flags;
    uint32_t id;
    uint32_t unused0;
    uint32_t unused1;
};
static_assert(sizeof(Record) == 24, "sizeof(Record) == 24");

struct GrupRecord {
    RecordType type;
    uint32_t group_size;
    uint32_t label;
    uint32_t group_type;
    uint32_t unused0;
    uint32_t unused1;
};
static_assert(sizeof(GrupRecord) == 24, "sizeof(GrupRecord) == 24");

constexpr uint32_t fourcc(char const p[5]) {
    return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

enum class RecordFieldType : uint32_t {
    FULL = 1280070982,
    BOD2 = 0x32444f42,
    BODT = 0x54444f42,
    BMDT = 0x54444d42,
    DNAM = 0x4d414e44,
    KSIZ = 0x5a49534b,
    KWDA = 0x4144574b,
    MAST = 0x5453414d,
    CNAM = fourcc("CNAM"),
    INTV = fourcc("INTV"),
    HEDR = fourcc("HEDR"),
    EDID = fourcc("EDID"),
    OBND = fourcc("OBND"),
    MODL = fourcc("MODL"),
};

#pragma pack(push, 1)
struct RecordField {
    RecordFieldType type;
    uint16_t size;
};
#pragma pack(pop)
static_assert(sizeof(RecordField) == 6, "sizeof(RecordField) == 6");

enum class TypeKind {
    Unknown,
    Struct,
    Float,
    Integer,
    ByteArray,
    ZString,
    LString,
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
    constexpr TypeStruct(const char* name, size_t size, TypeStructField(&fields)[N]) : Type(TypeKind::Struct, name, size), field_count(N), fields(fields) { }
};

extern Type Type_ZString;
extern Type Type_LString;
extern Type Type_ByteArray;
extern Type Type_float;
extern TypeInteger Type_int32;
extern TypeInteger Type_int16;
extern TypeStruct Type_Vector3;

struct RecordFieldDef {
    RecordFieldType type;
    const Type* data_type;
    const char* comment = nullptr;
};

struct RecordDef {
    RecordType type;
    const char* comment = nullptr;
    int field_count = 0;
    const RecordFieldDef* fields = nullptr;

    const RecordFieldDef* get_field_def(RecordFieldType type);
};

extern RecordDef Record_TES4;
extern RecordDef Record_WEAP;
