#include "typeinfo.hpp"
#include "common.hpp"
#include <stdlib.h>
#include <string.h>

#define CONCAT(a, b) a##b

#define rf_field(m_type, m_name, m_data_type)                       \
    ([]() -> const RecordFieldDef* {                                \
        static RecordFieldDef field{ m_type, m_data_type, m_name }; \
        return &field;                                              \
    })()

#define rf_zstring(m_type, m_name)      rf_field(m_type, m_name, &Type_ZString)
#define rf_lstring(m_type, m_name)      rf_field(m_type, m_name, &Type_LString)
#define rf_float(m_type, m_name)        rf_field(m_type, m_name, &Type_float)
#define rf_int8(m_type, m_name)         rf_field(m_type, m_name, &Type_int8_t)
#define rf_int16(m_type, m_name)        rf_field(m_type, m_name, &Type_int16_t)
#define rf_int32(m_type, m_name)        rf_field(m_type, m_name, &Type_int32_t)
#define rf_int64(m_type, m_name)        rf_field(m_type, m_name, &Type_int64_t)
#define rf_uint8(m_type, m_name)        rf_field(m_type, m_name, &Type_uint8_t)
#define rf_uint16(m_type, m_name)       rf_field(m_type, m_name, &Type_uint16_t)
#define rf_uint32(m_type, m_name)       rf_field(m_type, m_name, &Type_uint32_t)
#define rf_uint64(m_type, m_name)       rf_field(m_type, m_name, &Type_uint64_t)
#define rf_formid(m_type, m_name)       rf_field(m_type, m_name, &Type_FormID)
#define rf_formid_array(m_type, m_name) rf_field(m_type, m_name, &Type_FormIDArray)
#define rf_bytes(m_type, m_name)        rf_field(m_type, m_name, &Type_ByteArray)
#define rf_compressed(m_type, m_name)   rf_field(m_type, m_name, &Type_ByteArrayCompressed)
#define rf_bytes_rle(m_type, m_name)    rf_field(m_type, m_name, &Type_ByteArrayRLE)
#define rf_bool(m_type, m_name)         rf_field(m_type, m_name, &Type_bool)

#define rf_struct(m_type, m_name, m_size, ...)                \
    ([]() -> const RecordFieldDef* {                          \
        static TypeStructField fields[]{ __VA_ARGS__ };       \
        static TypeStruct type{ m_name, m_size, fields };     \
        static RecordFieldDef field{ m_type, &type, m_name }; \
        return &field;                                        \
    })()

#define rf_enum(m_type, m_name, m_size, m_flags, ...)            \
    ([]() -> const RecordFieldDef* {                             \
        static TypeEnumField fields[]{ __VA_ARGS__ };            \
        static TypeEnum type{ m_name, m_size, fields, m_flags }; \
        static RecordFieldDef field{ m_type, &type, m_name };    \
        return &field;                                           \
    })()

#define rf_enum_uint8(m_type, m_name, ...)   rf_enum(m_type, m_name, 1, false, __VA_ARGS__)
#define rf_enum_uint16(m_type, m_name, ...)  rf_enum(m_type, m_name, 2, false, __VA_ARGS__)
#define rf_enum_uint32(m_type, m_name, ...)  rf_enum(m_type, m_name, 4, false, __VA_ARGS__)
#define rf_flags_uint8(m_type, m_name, ...)  rf_enum(m_type, m_name, 1, true,  __VA_ARGS__)
#define rf_flags_uint16(m_type, m_name, ...) rf_enum(m_type, m_name, 2, true,  __VA_ARGS__)
#define rf_flags_uint32(m_type, m_name, ...) rf_enum(m_type, m_name, 4, true,  __VA_ARGS__)

#define rf_subrecord(...)                                                     \
    ([]() -> const RecordFieldDefSubrecord* {                                 \
        static const RecordFieldDef* fields[]{ __VA_ARGS__ };                 \
        static RecordFieldDefSubrecord field{ { fields, _countof(fields) } }; \
        return &field;                                                        \
    })()

#define rf_constant(m_type, m_name, m_decltype, ...)                                               \
    ([]() -> const RecordFieldDef* {                                                               \
        static m_decltype the_value = { __VA_ARGS__ };                                             \
        static TypeConstant constant{ "Constant", sizeof(the_value), (const uint8_t*)&the_value }; \
        static RecordFieldDef field{ m_type, &constant, m_name };                                  \
        return &field;                                                                             \
    })()


#define rf_filter(m_type, m_name, m_inner, m_preprocess)      \
    ([]() -> const RecordFieldDef* {                          \
        static TypeFilter type{ m_inner, m_preprocess };      \
        static RecordFieldDef field{ m_type, &type, m_name }; \
        return &field;                                        \
    })()

constexpr TypeStructField sf_int8(const char* name) {
    return { &Type_int8_t, name };
}

constexpr TypeStructField sf_int16(const char* name) {
    return { &Type_int16_t, name };
}

constexpr TypeStructField sf_int32(const char* name) {
    return { &Type_int32_t, name };
}

constexpr TypeStructField sf_int64(const char* name) {
    return { &Type_int64_t, name };
}

constexpr TypeStructField sf_uint8(const char* name) {
    return { &Type_uint8_t, name };
}

constexpr TypeStructField sf_uint16(const char* name) {
    return { &Type_uint16_t, name };
}

constexpr TypeStructField sf_uint32(const char* name) {
    return { &Type_uint32_t, name };
}

constexpr TypeStructField sf_uint64(const char* name) {
    return { &Type_uint64_t, name };
}

constexpr TypeStructField sf_float(const char* name) {
    return { &Type_float, name };
}

constexpr TypeStructField sf_formid(const char* name) {
    return { &Type_FormID, name };
}

constexpr TypeStructField sf_bool(const char* name) {
    return { &Type_bool, name };
}

constexpr TypeStructField sf_vector3(const char* name) {
    return { &Type_Vector3, name };
}

#define sf_constant(m_decltype, ...)                                                               \
    ([]() -> TypeStructField {                                                                     \
        static m_decltype the_value = { __VA_ARGS__ };                                             \
        static TypeConstant constant{ "Constant", sizeof(the_value), (const uint8_t*)&the_value }; \
        return { &constant, "Constant" };                                                          \
    })()

#define sf_constant_with_fallback(m_name, m_fallback, m_decltype, ...)                                     \
    ([]() -> TypeStructField {                                                                             \
        static m_decltype the_value = { __VA_ARGS__ };                                                     \
        static TypeConstant constant{ m_name, sizeof(the_value), (const uint8_t*)&the_value, m_fallback }; \
        return { &constant, m_name };                                                                      \
    })()

#define sf_constant_array(m_decltype, m_count, ...)                                               \
    ([]() -> TypeStructField {                                                                    \
        static m_decltype the_value[m_count] = { __VA_ARGS__ };                                   \
        static TypeConstant constant{ "Constant", sizeof(the_value), (const uint8_t*)the_value }; \
        return { &constant, "Constant" };                                                         \
    })()

#define sf_constant_array_with_fallback(m_name, m_decltype, m_count, ...)                                \
    ([]() -> TypeStructField {                                                                           \
        static m_decltype the_value[m_count] = { __VA_ARGS__ };                                          \
        static Type fallback{ TypeKind::ByteArrayFixed, m_name, m_count };                               \
        static TypeConstant constant{ m_name, sizeof(the_value), (const uint8_t*)the_value, &fallback }; \
        return { &constant, m_name };                                                                    \
    })()

template<size_t N>
constexpr TypeStructField sf_fixed_bytes(const char* name) {
    static Type Type_ByteArrayFixed{ TypeKind::ByteArrayFixed, name, N };
    return { &Type_ByteArrayFixed, name };
}

#define type_enum(m_size, m_flags, ...)                          \
    ([]() -> const TypeEnum* {                                   \
        static TypeEnumField fields[]{ __VA_ARGS__ };            \
        static TypeEnum type{ "enum", m_size, fields, m_flags }; \
        return &type;                                            \
    })()

#define type_enum_uint8(...)   type_enum(1, false, __VA_ARGS__)
#define type_enum_uint16(...)  type_enum(2, false, __VA_ARGS__)
#define type_enum_uint32(...)  type_enum(4, false, __VA_ARGS__)
#define type_flags_uint8(...)  type_enum(1, true,  __VA_ARGS__)
#define type_flags_uint16(...) type_enum(2, true,  __VA_ARGS__)
#define type_flags_uint32(...) type_enum(4, true,  __VA_ARGS__)

#define sf_enum(m_name, m_size, m_flags, ...)                    \
    ([]() -> TypeStructField {                                   \
        static TypeEnumField fields[]{ __VA_ARGS__ };            \
        static TypeEnum type{ m_name, m_size, fields, m_flags }; \
        return { &type, m_name };                                \
    })()

#define sf_enum_uint8(m_name, ...)   sf_enum(m_name, 1, false, __VA_ARGS__)
#define sf_enum_uint16(m_name, ...)  sf_enum(m_name, 2, false, __VA_ARGS__)
#define sf_enum_uint32(m_name, ...)  sf_enum(m_name, 4, false, __VA_ARGS__)
#define sf_flags_uint8(m_name, ...)  sf_enum(m_name, 1, true,  __VA_ARGS__)
#define sf_flags_uint16(m_name, ...) sf_enum(m_name, 2, true,  __VA_ARGS__)
#define sf_flags_uint32(m_name, ...) sf_enum(m_name, 4, true,  __VA_ARGS__)

#define sf_filter(m_name, m_inner, m_preprocess)         \
    ([]() -> TypeStructField {                           \
        static TypeFilter type{ m_inner, m_preprocess }; \
        return { &type, m_name };                        \
    })()

Type Type_ZString{ TypeKind::ZString, "CString", 0 };
Type Type_LString{ TypeKind::LString, "LString", 0 };
Type Type_WString{ TypeKind::WString, "WString", 0 };
Type Type_ByteArray{ TypeKind::ByteArray, "Byte Array", 0 };
Type Type_ByteArrayCompressed{ TypeKind::ByteArrayCompressed, "Byte Array (Compressed)", 0 };
Type Type_ByteArrayRLE{ TypeKind::ByteArrayRLE, "Byte Array (RLE)", 0 };
Type Type_float{ TypeKind::Float, "float", sizeof(float) };
Type Type_FormID{ TypeKind::FormID, "Form ID", sizeof(int) };
Type Type_FormIDArray{ TypeKind::FormIDArray, "Form ID Array", 0 };
Type Type_bool{ TypeKind::Boolean, "bool", sizeof(bool) };
Type Type_VMAD{ TypeKind::VMAD, "VMAD", 0 };
Type Type_NVPP{ TypeKind::NVPP, "NVPP", 0 };
Type Type_VTXT{ TypeKind::VTXT, "VTXT", 0 };
Type Type_XCLW{ TypeKind::XCLW, "XCLW", sizeof(float) };
Type Type_CTDA{ TypeKind::CTDA, "CTDA", 0 };
TypeInteger Type_int8_t{ "int8", sizeof(int8_t), false };
TypeInteger Type_int16_t{ "int16", sizeof(int16_t), false };
TypeInteger Type_int32_t{ "int32", sizeof(int32_t), false };
TypeInteger Type_int64_t{ "int64", sizeof(int64_t), false };
TypeInteger Type_uint8_t{ "uint8", sizeof(uint8_t), true };
TypeInteger Type_uint16_t{ "uint16", sizeof(uint16_t), true };
TypeInteger Type_uint32_t{ "uint32", sizeof(uint32_t), true };
TypeInteger Type_uint64_t{ "uint64", sizeof(uint64_t), true };
Type Type_Vector3{ TypeKind::Vector3, "Vector3", sizeof(Vector3) };

static auto Field_MODL = rf_zstring("MODL", "Model File Name");

const RecordFieldDefBase* RecordDef::get_field_def(RecordFieldType type) const {
    for (int i = 0; i < fields.count; ++i) {
        const auto field = fields.data[i];
        if (field->type == type) {
            return field;
        }
    }
    return nullptr;
}

#define TYPE_ENUM(m_type, m_name, m_size, ...)           \
    static TypeEnumField CONCAT(Type_, m_type)_Fields[]{ \
        __VA_ARGS__                                      \
    };                                                   \
                                                         \
    TypeEnum Type_##m_type{                              \
        m_name,                                          \
        m_size,                                          \
        CONCAT(Type_, m_type)_Fields,                    \
        false                                            \
    }

#define TYPE_FLAGS(m_type, m_name, m_size, ...)           \
    static TypeEnumField CONCAT(Type_, m_type)_Fields[]{ \
        __VA_ARGS__                                      \
    };                                                   \
                                                         \
    TypeEnum Type_##m_type{                              \
        m_name,                                          \
        m_size,                                          \
        CONCAT(Type_, m_type)_Fields,                    \
        true                                             \
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

TYPE_STRUCT(CNTO, "Item", 8,
    sf_formid("Item"),
    sf_uint32("Count"),
);

TYPE_ENUM(PapyrusPropertyType, "Papyrus Property Type", sizeof(PapyrusPropertyType),
    { (uint32_t)PapyrusPropertyType::Object, "Object" },
    { (uint32_t)PapyrusPropertyType::String, "String" },
    { (uint32_t)PapyrusPropertyType::Int, "Int" },
    { (uint32_t)PapyrusPropertyType::Float, "Float" },
    { (uint32_t)PapyrusPropertyType::Bool, "Bool" },
    { (uint32_t)PapyrusPropertyType::ObjectArray, "Object[]" },
    { (uint32_t)PapyrusPropertyType::StringArray, "String[]" },
    { (uint32_t)PapyrusPropertyType::IntArray, "Int[]" },
    { (uint32_t)PapyrusPropertyType::FloatArray, "Float[]" },
    { (uint32_t)PapyrusPropertyType::BoolArray, "Bool[]" },
);

TYPE_FLAGS(PapyrusFragmentFlags, "Papyrus Fragment Flags", sizeof(PapyrusFragmentFlags),
    { (uint32_t)PapyrusFragmentFlags::HasBeginScript, "Has Begin Script" },
    { (uint32_t)PapyrusFragmentFlags::HasEndScript, "Has End Script" },
);

TYPE_FLAGS(CTDA_Flags, "CTDA_Flags", sizeof(CTDA_Flags),
    { (uint32_t)CTDA_Flags::Or, "Or" },
    { (uint32_t)CTDA_Flags::ParametersUseAliases, "Parameters (Use Aliases)" },
    { (uint32_t)CTDA_Flags::UseGlobal, "Use Global" },
    { (uint32_t)CTDA_Flags::UsePackData, "Use Pack Data" },
    { (uint32_t)CTDA_Flags::SwapSubjectAndTarget, "Swap Subject And Target" },
);


TYPE_ENUM(CTDA_RunOnType, "CTDA_RunOnType", sizeof(CTDA_RunOnType),
    { (uint32_t)CTDA_RunOnType::Subject, "Subject" },
    { (uint32_t)CTDA_RunOnType::Target, "Target" },
    { (uint32_t)CTDA_RunOnType::Reference, "Reference" },
    { (uint32_t)CTDA_RunOnType::CombatTarget, "Combat Target" },
    { (uint32_t)CTDA_RunOnType::LinkedReference, "Linked Reference" },
    { (uint32_t)CTDA_RunOnType::QuestAlias, "Quest Alias" },
    { (uint32_t)CTDA_RunOnType::PackageData, "Package Data" },
    { (uint32_t)CTDA_RunOnType::EventData, "Event Data" },
);

TYPE_FLAGS(VMAD_PACK_Flags, "VMAD_PACK_Flags", sizeof(VMAD_PACK_Flags),
    { (uint32_t)VMAD_PACK_Flags::OnBegin, "On Begin" },
    { (uint32_t)VMAD_PACK_Flags::OnEnd, "On End" },
    { (uint32_t)VMAD_PACK_Flags::OnChange, "On Change" },
);

TYPE_FLAGS(ProgramOptions, "Program Options", sizeof(ProgramOptions),
    { (uint32_t)ProgramOptions::ExportTimestamp, "Export Timestamp" },
    { (uint32_t)ProgramOptions::PreserveOrder, "Preserve Order" },
    { (uint32_t)ProgramOptions::PreserveJunk, "Preserve Junk" },
);

constexpr RecordType record_type(const char p[5]) {
    return (RecordType)fourcc(p);
}

#define record_flags(...)                             \
    ([]() -> StaticArray<RecordFlagDef> {             \
        static RecordFlagDef fields[]{ __VA_ARGS__ }; \
        return { fields, _countof(fields) };          \
    })()

#define record_fields(...)                                    \
    ([]() -> StaticArray<const RecordFieldDefBase*> {             \
        static const RecordFieldDefBase* fields[]{ __VA_ARGS__ }; \
        return { fields, _countof(fields) };                  \
    })()

RecordDef Record_Common{
    .type = (RecordType)0,
    .comment = "-- common --",
    .fields = record_fields(
        rf_zstring("EDID", "Editor ID"),
        rf_lstring("FULL", "Name"),
        rf_struct("OBND", "Object Bounds", 12,
            sf_int16("X1"),
            sf_int16("Y1"),
            sf_int16("Z1"),
            sf_int16("X2"),
            sf_int16("Y2"),
            sf_int16("Z2"),
        ),
        rf_int32("COCT", "Item Count"),
        rf_field("CNTO", "Items", &Type_CNTO),
        rf_field("VMAD", "Script", &Type_VMAD),
        rf_int32("KSIZ", "Keyword Count"),
        rf_formid_array("KWDA", "Keywords"),
        rf_zstring("FLTR", "Object Window Filter"),
    ),
    .flags = record_flags(
        { 0x20, "Deleted" },
        { RecordFlags::Compressed, "Compressed" },
        { 0x800000, "Is Marker" },
        { 0x8000000, "NavMesh Generation - Bounding Box" },
    ),
};

static RecordDef Record_TES4{
    .type = record_type("TES4"),
    .comment = "File Header",
    .fields = record_fields(
        rf_struct("HEDR", "Header", 12,
            sf_float("Version"),
            sf_int32("Number Of Records"),
            sf_formid("Next Object ID"),
        ),
        rf_subrecord(
            rf_zstring("MAST", "Master File"),
            rf_constant("DATA", "Unused", uint64_t, 0),
        ),
        rf_zstring("CNAM", "Author"),
        rf_uint32("INTV", "Tagified Strings"),
        rf_zstring("SNAM", "Description"),
        rf_formid_array("ONAM", "Overriden Forms"),
    ),
    .flags = record_flags(
        { RecordFlags::TES4_Master, "Master" },
        { RecordFlags::TES4_Localized, "Localized" },
        { 0x200, "Light Master" },
    ),
};

static RecordDef Record_WEAP{
    .type = record_type("WEAP"),
    .comment = "Weapon",
    .fields = record_fields(
        rf_formid("ETYP", "Equipment Type"),
        rf_formid("BIDS", "Block Bash Impact Data Set"),
        rf_formid("BAMT", "Alternate Block Material"),
        rf_lstring("DESC", "Description"),
        rf_formid("INAM", "Impact Data Set"),
        rf_formid("WNAM", "1st Person Model Object"),
        rf_formid("TNAM", "Attack Fail Sound"),
        rf_formid("NAM9", "Equip Sound"),
        rf_formid("NAM8", "Unequip Sound"),
        rf_struct("DATA", "Game Data", 10,
            sf_int32("Value"),
            sf_float("Weight"),
            sf_int16("Damage"),
        ),
        rf_struct("DNAM", "Weapon Data", 100,
            sf_uint8("Animation Type"),
            sf_int8("Unknown 0"),
            sf_int16("Unknown 1"),
            sf_float("Speed"),
            sf_float("Reach"),
            sf_uint16("Flags"),
            sf_uint16("Flags?"),
            sf_float("Sight FOV"),
            sf_constant(uint32_t, 0),
            sf_uint8("VATS to hit"),
            sf_constant(int8_t, -1),
            sf_uint8("Projectiles"),
            sf_int8("Embedded Weapon"),
            sf_float("Min Range"),
            sf_float("Max Range"),
            sf_constant(uint32_t, 0),
            sf_uint32("Flags"),
            sf_constant(float, 1.0f),
            sf_float("Unknown"),
            sf_float("Rumble Left"),
            sf_float("Rumble Right"),
            sf_float("Rumble Duration"),
            sf_constant_array(uint32_t, 3, 0, 0, 0),
            sf_int32("Skill"),
            sf_constant_array(uint32_t, 2, 0, 0),
            sf_int32("Resist"),
            sf_constant(uint32_t, 0),
            sf_float("Stagger"),
        ),
        rf_struct("CRDT", "Critical Data", 24,
            sf_uint16("Critical Damage"),
            sf_uint16("Unknown"),
            sf_float("Critical % Mult"),
            sf_uint32("Flags"),
            sf_uint32("Unknown"),
            sf_formid("Critical Spell Effect"),
            sf_uint32("Unknown"),
        ),
        rf_int32("VNAM", "Detection Sound Level"),
        Field_MODL,
    ),
};

static RecordDef Record_QUST{
    .type = record_type("QUST"),
    .comment = "Quest",
    .fields = record_fields(
        rf_struct("DNAM", "Quest Data", 12,
            sf_flags_uint16("Flags",
                { 0x001, "Start Game Enabled" },
                { 0x004, "Wilderness Encounter" },
                { 0x008, "Allow Repeated Stages" },
                { 0x100, "Run Once" },
                { 0x200, "Exclude From Dialogue Export" },
                { 0x400, "Warn On Alias Fill Failure" },
            ),
            sf_uint8("Priority"),
            sf_uint8("Unknown"),
            sf_constant(uint32_t, 0),
            sf_enum_uint32("Type",
                { 0x0, "None" },
                { 0x1, "Main Quest" },
                { 0x2, "Mages Guild" },
                { 0x3, "Thieves Guild" },
                { 0x4, "Dark Brotherhood" },
                { 0x5, "Companion Quests" },
                { 0x6, "Miscellaneous" },
                { 0x7, "Daedric Quests" },
                { 0x8, "Side Quests" },
                { 0x9, "Civil War" },
                { 0xA, "DLC01 - Vampire" },
                { 0xB, "DLC02 - Dragonborn" },
            ),
        ),
        rf_struct("INDX", "Index", 4,
            sf_uint16("Journal Index"),
            sf_flags_uint8("Flags",
                { 0x2, "Start Up Stage" },
                { 0x4, "Shut Down Stage" },
                { 0x8, "Keep Instance Data From Here On" },
            ),
            sf_int8("Unknown"),
        ),
        rf_lstring("CNAM", "Journal Entry"),
        rf_flags_uint8("QSDT", "Flags",
            { 0x1, "Complete Quest" },
            { 0x2, "Fail Quest" },
        ),
        rf_int16("QOBJ", "Objective Index"),
        rf_uint32("FNAM", "Objective Flags"),
        rf_lstring("NNAM", "Objective Text"),
        rf_struct("QSTA", "Quest Target", 8,
            sf_int32("Target Alias"),
            sf_int32("Flags"),
        ),
        rf_uint32("ANAM", "Next Alias ID"),
        rf_uint32("ALST", "Alias ID"),
        rf_uint32("ALLS", "Location Alias ID"),
        rf_zstring("ALID", "Alias Name"),
        rf_formid("ALFR", "Alias Forced Reference"),
        rf_formid("ALUA", "Alias Unique Actor"),
        rf_formid("VTCK", "Voice Type"),
        rf_formid("ALCO", "Alias Created Object"),
        rf_uint32("ALCA", "Create At/In"), // @TODO high-low bit thing
        rf_enum_uint32("ALCL", "Create Level",
            { 0, "Easy" },
            { 1, "Medium" },
            { 2, "Hard" },
            { 3, "Very Hard" },
            { 4, "None" },
        ),
    ),
};

static void sort_formid_array(void* data, size_t size) {
    verify((size % sizeof(FormID)) == 0);
    qsort(data, size / sizeof(FormID), sizeof(FormID), [](void const* aa, void const* bb) -> int {
        FormID a = *(FormID*)aa;
        FormID b = *(FormID*)bb;
        return a.value - b.value;
    });
}

static RecordDef Record_CELL{
    .type = record_type("CELL"),
    .comment = "Cell",
    .fields = record_fields(
        rf_flags_uint16("DATA", "Flags",
            { 0x001, "Interior" },
            { 0x002, "Has Water" },
            { 0x004, "Can't Travel From Here" },
            { 0x008, "No LOD Water" },
            { 0x020, "Public Area" },
            { 0x040, "Hand Changed" }, // ???
            { 0x080, "Show Sky" },
            { 0x100, "Use Sky Lighting" },
        ),
        rf_struct("XCLC", "Data", 12,
            sf_int32("X"),
            sf_int32("Y"),
            sf_filter("Flags",
                type_flags_uint32(
                    { 0x1, "Force Hide Land Quad 1" },
                    { 0x2, "Force Hide Land Quad 2" },
                    { 0x4, "Force Hide Land Quad 3" },
                    { 0x8, "Force Hide Land Quad 4" },
                ),
                [](void* data, size_t size) -> void {
                    verify(size == 4);
                    uint32_t* flags = (uint32_t*)data;
                    *flags &= 0b00000000'00000000'00000000'00001111;
                }
            ),
        ),
        rf_formid("LTMP", "Lighting Template"),
        rf_filter("XCLR", "Regions Containing Cell",
            &Type_FormIDArray,
            sort_formid_array
        ),
        rf_formid("XLCN", "Location"),
        rf_formid("XCWT", "Water"),
        rf_compressed("TVDT", "Occlusion Data"),
        rf_compressed("MHDT", "MHDT"),
        rf_struct("XCLL", "Lighting", 92,
            sf_fixed_bytes<4>("Ambient Color"),
            sf_fixed_bytes<4>("Directional Color"),
            sf_fixed_bytes<4>("Fog Near Color"),
            sf_float("Fog Near"),
            sf_float("Fog Far"),
            sf_int32("Rotation XY"),
            sf_int32("Rotation Z"),
            sf_float("Directional Fade"),
            sf_float("Fog Clip Distance"),
            sf_float("Fow Pow"),
            sf_fixed_bytes<4>("Ambient X+ Color"),
            sf_fixed_bytes<4>("Ambient X- Color"),
            sf_fixed_bytes<4>("Ambient Y+ Color"),
            sf_fixed_bytes<4>("Ambient Y- Color"),
            sf_fixed_bytes<4>("Ambient Z+ Color"),
            sf_fixed_bytes<4>("Ambient Z- Color"),
            sf_fixed_bytes<4>("Specular Color"),
            sf_float("Fresnel Power"),
            sf_fixed_bytes<4>("Fog Far Color"),
            sf_float("Fog Max"),
            sf_float("Light Fade Distance Start"),
            sf_float("Light Fade Distance End"),
            sf_flags_uint32("Inheritance Flags",
                { 0x001, "Ambient Color" },
                { 0x002, "Directional Color" },
                { 0x004, "Fog Color" },
                { 0x008, "Fog Near" },
                { 0x010, "Fog Far" },
                { 0x020, "Directional Rotation" },
                { 0x040, "Directional Fade" },
                { 0x080, "Clip Distance" },
                { 0x100, "Fog Power" },
                { 0x200, "Fog Max" },
                { 0x400, "Light Fade Distance" },
            ),
        ),
        rf_formid("XEZN", "Encounter Zone"),
        rf_formid("XCAS", "Acoustic Space"),
        rf_formid("XCMO", "Music Type"),
        rf_formid("XCIM", "Image Space"),
        rf_formid("XCCM", "Climate"),
        rf_formid("XOWN", "Owner"),
        rf_zstring("XWEM", "Water Environment Map"),
        rf_field("XCLW", "Water Level", &Type_XCLW),
    ),
    .flags = record_flags(
        { 0x400, "Persistent" },
    ),
};

auto Type_LocationData = rf_struct("DATA", "Data", 24,
    sf_vector3("Pos XYZ"),
    sf_vector3("Rot XYZ"),
);

static RecordDef Record_REFR{
    .type = record_type("REFR"),
    .comment = "Reference",
    .fields = record_fields(
        rf_formid("NAME", "Base Form ID"),
        rf_float("XSCL", "Scale"),
        rf_flags_uint8("XAPD", "Activation Parent Flags",
            { 0x1, "Parent Activate Only" },
        ),
        rf_struct("XAPR", "Activation Parent", 8,
            sf_formid("Form ID"),
            sf_float("Delay"),
        ),
        rf_flags_uint8("FNAM", "Marker Flags",
            { 0x1, "Visible" },
            { 0x2, "Can Travel To" },
            { 0x4, "Show All" },
        ),
        rf_struct("XNDP", "Door Pivot", 8,
            sf_formid("NavMesh"),
            sf_uint16("NavMesh Triangle Index"),
            sf_constant_with_fallback("Unknown", &Type_uint16_t, uint16_t, 0),
        ),
        rf_struct("XLKR", "Linked Reference", 8,
            sf_formid("Keyword"),
            sf_formid("Reference"),
        ),
        rf_struct("XTEL", "Door Teleport", 32,
            sf_formid("Destination Door"),
            sf_vector3("Pos XYZ"),
            sf_vector3("Rot XYZ"),
            sf_flags_uint32("Flags",
                { 0x1, "No Alarm" },
            ),
        ),
        rf_formid("XLRL", "Location"),
        rf_bytes_rle("XRGD", "Ragdoll Data"),
        Type_LocationData,
        rf_uint32("XTRI", "Collision Layer"),
    ),
    .flags = record_flags(
        { 0x00000200, "Hidden From Local Map" },
        { 0x00000400, "Persistent" },
        { 0x00000800, "Initially Disabled" },
        { 0x00010000, "Is Full LOD" },
        { 0x02000000, "No AI Acquire" },
        { 0x10000000, "Reflected By Auto Water" },
        { 0x20000000, "Don't Havok Settle" },
        { 0x40000000, "Not Respawns" },
    ),
};

static RecordDef Record_CONT{
    .type = record_type("CONT"),
    .comment = "Container",
    .fields = record_fields(
        rf_struct("DATA", "Data", 5,
            sf_uint8("Flags"),
            sf_float("Unknown"),
        ),
        Field_MODL,
    ),
};

static RecordDef Record_NPC_{
    .type = record_type("NPC_"),
    .comment = "Non-Player Character",
    .fields = record_fields(
        rf_struct("ACBS", "Base Stats", 24,
            sf_flags_uint32("Flags",
                { 0x00000001, "Female" },
                { 0x00000002, "Essential" },
                { 0x00000004, "Is CharGen Face Preset" },
                { 0x00000008, "Respawn" },
                { 0x00000010, "Auto Calc Stats" },
                { 0x00000020, "Unique" },
                { 0x00000040, "Doesn't Affect Stealth Meter" },
                { 0x00000080, "PC Level Mult" },
                { 0x00000100, "Audio Template" },
                { 0x00000800, "Protected" },
                { 0x00004000, "Summonable" },
                { 0x00010000, "Doesn't Bleed" },
                { 0x00040000, "Owned/Follow" },
                { 0x00080000, "Opposite Gender Anims" },
                { 0x00100000, "Simple Actor" },
                { 0x00200000, "Looped Script" },
                { 0x10000000, "Looped Audio" },
                { 0x20000000, "Ghost/Non-Interactable" },
                { 0x80000000, "Invulnerable" },
            ),
            sf_int16("Magicka Offset"),
            sf_int16("Stamina Offset"),
            sf_uint16("Level"),
            sf_uint16("Calc Min Level"),
            sf_uint16("Calc Max Level"),
            sf_uint16("Speed Multiplier"),
            sf_uint16("Disposition Base"),
            sf_flags_uint16("Template Data Flags",
                { 0x0001, "Use Traits" },
                { 0x0002, "Use Stats" },
                { 0x0004, "Use Factions" },
                { 0x0008, "Use Spell List" },
                { 0x0010, "Use AI Data" },
                { 0x0020, "Use AI Packages" },
                { 0x0040, "Unknown 0x40" },
                { 0x0080, "Use Base Data" },
                { 0x0100, "Use Inventory" },
                { 0x0200, "Use Script" },
                { 0x0400, "Use Def Pack List" },
                { 0x0800, "Use Attack Data" },
                { 0x1000, "Use Keywords" },
            ),
            sf_int16("Health Offset"),
            sf_uint16("Bleedout Override"),
        ),
        rf_formid("VTCK", "Voice Type"),
        rf_formid("TPLT", "Template"),
        rf_formid("RACE", "Race"),
        rf_formid("ATKR", "Attack Race"),
        rf_formid("PNAM", "Head Part"),
        rf_formid("HCLF", "Hair Color"),
        rf_formid("ZNAM", "Combat Style"),
        rf_float("NAM6", "Height"),
        rf_float("NAM7", "Weight"),
        rf_enum_uint32("NAM8", "Sound Level",
            { 0, "Loud" },
            { 1, "Normal" },
            { 2, "Silent" },
            { 3, "Very Loud" },
        ),
        rf_formid("DOFT", "Default Outfit"),
        rf_formid("DPLT", "Default Package List"),
        rf_formid("FTST", "Face Texture Set"),
        rf_struct("NAM9", "Face Morph", 76,
            sf_float("Nose Long/Short"),
            sf_float("Nose Up/Down"),
            sf_float("Jaw Up/Down"),
            sf_float("Jaw Narrow/Wide"),
            sf_float("Jaw Forward/Back"),
            sf_float("Cheeks Up/Down"),
            sf_float("Cheeks Forward/Back"),
            sf_float("Eyes Up/Down"),
            sf_float("Eyes In/Out"),
            sf_float("Brows Up/Down"),
            sf_float("Brows In/Out"),
            sf_float("Brows Forward/Back"),
            sf_float("Lips Up/Down"),
            sf_float("Lips In/Out"),
            sf_float("Chin Thin/Wide"),
            sf_float("Chin Up/Down"),
            sf_float("Chin Underbite/Overbite"),
            sf_float("Eyes Forward/Back"),
            sf_uint32("Unknown"),
        ),
        rf_formid("RNAM", "Race"),
        rf_uint32("PRKZ", "Perk Count"),
        rf_struct("PRKR", "Perk", 8,
            sf_formid("Perk"),
            sf_uint32("Unknown"),
        ),
        rf_struct("AIDT", "AI Data", 20,
            sf_uint8("Aggression"),
            sf_uint8("Confidence"),
            sf_uint8("Energy"),
            sf_uint8("Morality"),
            sf_uint8("Mood"),
            sf_uint8("Assistance"),
            sf_uint8("Flags"),
            sf_uint8("Unknown"),
            sf_uint32("Warn"),
            sf_uint32("Warn/Attack"),
            sf_uint32("Attack"),
        ),
        rf_formid("PKID", "AI Package"),
        rf_formid("CNAM", "Class"),
        rf_struct("DNAM", "Data", 52,
            sf_uint8("Base Skill - One-Handed"),
            sf_uint8("Base Skill - Two-Handed"),
            sf_uint8("Base Skill - Marksman"),
            sf_uint8("Base Skill - Block"),
            sf_uint8("Base Skill - Smithing"),
            sf_uint8("Base Skill - Heavy Armor"),
            sf_uint8("Base Skill - Light Armor"),
            sf_uint8("Base Skill - Pickpocket"),
            sf_uint8("Base Skill - Lockpicking"),
            sf_uint8("Base Skill - Sneak"),
            sf_uint8("Base Skill - Alchemy"),
            sf_uint8("Base Skill - Speechcraft"),
            sf_uint8("Base Skill - Alteration"),
            sf_uint8("Base Skill - Conjuration"),
            sf_uint8("Base Skill - Destruction"),
            sf_uint8("Base Skill - Illusion"),
            sf_uint8("Base Skill - Restoration"),
            sf_uint8("Base Skill - Enchanting"),
            sf_constant_with_fallback("Mod Skill - One-Handed", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Two-Handed", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Marksman", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Block", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Smithing", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Heavy Armor", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Light Armor", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Pickpocket", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Lockpicking", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Sneak", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Alchemy", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Speechcraft", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Alteration", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Conjuration", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Destruction", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Illusion", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Restoration", &Type_uint8_t, uint8_t, 0),
            sf_constant_with_fallback("Mod Skill - Enchanting", &Type_uint8_t, uint8_t, 0),
            sf_uint16("Calculated Health"),
            sf_uint16("Calculated Magicka"),
            sf_uint16("Calculated Stamina"),
            sf_uint16("Unknown"),
            sf_float("Far Away Model Distance"),
            sf_uint8("Geared Up Weapons"),
            sf_fixed_bytes<3>("Unknown"),
        ),
        rf_struct("QNAM", "Skin Tone", 12,
            sf_float("Red"),
            sf_float("Green"),
            sf_float("Blue"),
        ),
        rf_struct("NAMA", "Face Parts", 16,
            sf_int32("Nose"),
            sf_int32("Unknown"),
            sf_int32("Eyes"),
            sf_int32("Mouth"),
        ),
        rf_uint16("TINI", "Tint Item"),
        rf_struct("TINC", "Tint Color", 4,
            sf_uint8("Red"),
            sf_uint8("Green"),
            sf_uint8("Blue"),
            sf_constant(uint8_t, 0),
        ),
        rf_int32("TINV", "Tint Value"),
        rf_struct("SNAM", "Faction", 8,
            sf_formid("Faction"),
            sf_int8("Rank"),
            sf_constant_array(uint8_t, 3, 0x00, 0x00, 0x00),
        ),
        rf_formid("CRIF", "Crime Faction"),
    ),
};

static void sort_nvpp(void* data, size_t size) {
    BinaryReader r{ (uint8_t*)data, size };

    const auto path_count = r.read<uint32_t>();
    for (uint32_t i = 0; i < path_count; ++i) {
        const auto formid_count = r.read<uint32_t>();
        r.advance(sizeof(FormID) * formid_count);
    }

    const auto node_count = r.read<uint32_t>();
    struct Node {
        FormID formid;
        uint32_t index;
    };
    static_assert(sizeof(Node) == 8, "invalid Node size");
    
    auto nodes = (Node*)r.advance(sizeof(Node) * node_count);
    qsort(nodes, node_count, sizeof(nodes[0]), [](void const* aa, void const* bb) -> int {
        Node* a = (Node*)aa;
        Node* b = (Node*)bb;
        return a->index - b->index;
    });

    verify(r.now == r.end);
}

static RecordDef Record_NAVI{
    .type = record_type("NAVI"),
    .comment = "Navigation",
    .fields = record_fields(
        rf_uint32("NVER", "Version"),
        rf_bytes_rle("NVMI", "NavMesh Data"),
        rf_filter("NVPP", "Preferred Pathing Data",
            &Type_ByteArrayCompressed,
            sort_nvpp
        )
        // This is for debugging.
        //rf_field("NVPP", "Preferred Pathing Data", &Type_NVPP),
    ),
};

static RecordDef Record_DLVW{
    .type = record_type("DLVW"),
    .comment = "Dialogue View",
    .fields = record_fields(
        rf_formid("QNAM", "Parent Quest"),
        rf_formid("BNAM", "Branch"),
        rf_formid("TNAM", "Topic"),
        rf_uint32("ENAM", "Unknown"),
        rf_bool("DNAM", "Show All Text"),
    ),
};

static RecordDef Record_DLBR{
    .type = record_type("DLBR"),
    .comment = "Dialogue Branch",
    .fields = record_fields(
        rf_formid("QNAM", "Parent Quest"),
        rf_uint32("TNAM", "Unknown"),
        rf_uint32("DNAM", "Flags"),
        rf_formid("SNAM", "Start Dialogue"),
    ),
};

static RecordDef Record_INFO{
    .type = record_type("INFO"),
    .comment = "Topic Info",
    .fields = record_fields(
        rf_struct("ENAM", "Data", 4,
            sf_flags_uint16("Flags",
                { 0x0001, "Goodbye" },
                { 0x0002, "Random" },
                { 0x0004, "Say Once" },
                { 0x0010, "On Activation" },
                { 0x0020, "Random End" },
                { 0x0040, "Invisible Continue" },
                { 0x0080, "Walk Away" },
                { 0x0100, "Walk Away Invisible In Menu" },
                { 0x0200, "Force Subtitle" },
                { 0x0400, "Can Move While Greeting" },
                { 0x0800, "Has No Lip File" },
                { 0x1000, "Requires Post-Processing" },
                { 0x4000, "Has Audio Output Override" },
                { 0x8000, "Spends Favor Points" },
            ),
            sf_uint16("Hours Until Reset"),
        ),
        rf_formid("PNAM", "Previous Info"),
        rf_uint8("CNAM", "Favor Level"),
        rf_formid("TCLT", "Topic Links"),
        rf_lstring("NAM1", "Response"), // @TODO: ilstring
        rf_zstring("NAM2", "Notes"),
        rf_zstring("NAM3", "Edits"),
        rf_lstring("RNAM", "Player Response"),
        rf_struct("TRDT", "Response", 24,
            sf_enum_uint32("Emotion",
                { 0, "Neutral" },
                { 1, "Anger" },
                { 2, "Disgust" },
                { 3, "Fear" },
                { 4, "Sad" },
                { 5, "Happy" },
                { 6, "Surprise" },
                { 7, "Puzzled" },
            ),
            sf_uint32("Emotion Value"),
            sf_constant(uint32_t, 0),
            sf_uint8("Response Index"),
            sf_constant_array(uint8_t, 3, 0x00, 0x00, 0x00),
            sf_formid("Sound"),
            sf_bool("Use Emotion Animation"),
            sf_constant_array(uint8_t, 3, 0x00, 0x00, 0x00),
        ),
        rf_field("CTDA", "Condition", &Type_CTDA),
        rf_zstring("CIS1", "Condition Argument 1"),
        rf_zstring("CIS2", "Condition Argument 2"),
    ),
};

static RecordDef Record_ACHR{
    .type = record_type("ACHR"),
    .comment = "Actor",
    .fields = record_fields(
        rf_formid("NAME", "Base NPC"),
        rf_bytes("XRGD", "Ragdoll Data"),
        Type_LocationData,
    ),
    .flags = record_flags(
        { 0x00000200, "Starts Dead" },
        { 0x00000400, "Persistent Reference" },
        { 0x00000800, "Initially Disabled" },
        { 0x02000000, "No AI Acquire" },
        { 0x10000000, "Reflected By Auto Water" },
        { 0x20000000, "Don't Havok Settle" },
    ),
};

static RecordDef Record_DIAL{
    .type = record_type("DIAL"),
    .comment = "Dialogue Topic",
    .fields = record_fields(
        rf_float("PNAM", "Priority"),
        rf_formid("BNAM", "Owning Branch"),
        rf_formid("QNAM", "Owning Quest"),
        rf_uint32("TIFC", "Info Count"),
        // @TODO: SNAM = char[4]
    ),
};

static RecordDef Record_KYWD{
    .type = record_type("KYWD"),
    .comment = "Keyword",
    .fields = record_fields(
        rf_struct("CNAM", "Color", 4,
            sf_uint8("Red"),
            sf_uint8("Green"),
            sf_uint8("Blue"),
            sf_constant(uint8_t, 0), // Alpha
        ),
    ),
};

static RecordDef Record_TXST{
    .type = record_type("TXST"),
    .comment = "Texture Set",
    .fields = record_fields(
        rf_zstring("TX00", "Color Map"),
        rf_zstring("TX01", "Normal Map"),
        rf_zstring("TX02", "Mask"),
        rf_zstring("TX03", "Tone Map"),
        rf_zstring("TX04", "Detail Map"),
        rf_zstring("TX05", "Environment Map"),
        rf_zstring("TX07", "Specularity Map"),
        rf_flags_uint16("DNAM", "Flags",
            { 0x02, "Facegen Textures" },
            { 0x04, "Has Model Space Normal Map" },
        ),
    ),
};

static RecordDef Record_GLOB{
    .type = record_type("GLOB"),
    .comment = "Global",
    .fields = record_fields(
        rf_enum_uint8("FNAM", "Type",
            { 's', "Short" },
            { 'l', "Long" },
            { 'f', "Float" },
        ),
        rf_float("FLTV", "Value"),
    ),
};

static RecordDef Record_FACT{
    .type = record_type("FACT"),
    .comment = "Faction",
    .fields = record_fields(
        rf_flags_uint32("DATA", "Flags",
            { 0x00001, "Hidden from PC" },
            { 0x00002, "Special Combat" },
            { 0x00040, "Track Crime" },
            { 0x00080, "Ignore Murder" },
            { 0x00100, "Ignore Assault" },
            { 0x00200, "Ignore Stealing" },
            { 0x00400, "Ignore Trespass" },
            { 0x00800, "Do not report crimes against members" },
            { 0x01000, "Crime Gold, Use Defaults" },
            { 0x02000, "Ignore Pickpocket" },
            { 0x04000, "Vendor" },
            { 0x08000, "Can be Owner" },
            { 0x10000, "Ignore Werewolf" },
        ),
        rf_uint32("RNAM", "Rank ID"),
        rf_lstring("MNAM", "Male Rank Title"),
        rf_lstring("FNAM", "Female Rank Title"),
    ),
};

static RecordDef Record_SOUN{
    .type = record_type("SOUN"),
    .comment = "Sound",
    .fields = record_fields(
        rf_formid("SDSC", "Sound Descriptor"),
    ),
};

static RecordDef Record_MGEF{
    .type = record_type("MGEF"),
    .comment = "Magic Effect",
    .fields = record_fields(
        rf_lstring("DNAM", "Description"),
    ),
};

static RecordDef Record_SPEL{
    .type = record_type("SPEL"),
    .comment = "Spell",
    .fields = record_fields(
        rf_formid("ETYP", "Equipment Type"),
        rf_lstring("DESC", "Description"),
        rf_formid("EFID", "Magic Effect Form ID"),
        rf_struct("EFIT", "Magic Effect", 12,
            sf_float("Magnitude"),
            sf_uint32("Area of Effect"),
            sf_uint32("Duration"),
        ),
    ),
};

static RecordDef Record_FLST{
    .type = record_type("FLST"),
    .comment = "Form List",
    .fields = record_fields(
        rf_formid("LNAM", "Object"),
    ),
};

static RecordDef Record_STAT{
    .type = record_type("STAT"),
    .comment = "Static",
    .fields = record_fields(
        rf_struct("DNAM", "Data", 8,
            sf_float("Max Angle"),
            sf_formid("Directional Material"),
        ),
        Field_MODL,
    ),
};

static RecordDef Record_MISC{
    .type = record_type("MISC"),
    .comment = "Misc Item",
    .fields = record_fields(
        Field_MODL,
    ),
};

static RecordDef Record_FURN{
    .type = record_type("FURN"),
    .comment = "Furniture",
    .fields = record_fields(
        Field_MODL,
        rf_zstring("XMRK", "Marker Model File Name"),
    ),
};

static RecordDef Record_WRLD{
    .type = record_type("WRLD"),
    .comment = "Worldspace",
    .fields = record_fields(
        rf_formid("CNAM", "Climate"),
        rf_formid("NAM2", "Water"),
        rf_formid("NAM3", "LOD Water Type"),
        rf_float("NAM4", "LOD Water Height"),
        rf_struct("DNAM", "Land Data", 8,
            sf_float("Default Land Level"),
            sf_float("Default Ocean Level"),
        ),
        rf_flags_uint8("DATA", "Flags",
            { 0x01, "Small World" },
            { 0x02, "Can't Fast Travel From Here" },
            { 0x08, "No LOD Water" },
            { 0x10, "No Landscape" },
            { 0x20, "No Sky" },
            { 0x40, "Fixed Dimensions" },
            { 0x80, "No Grass" },
        ),
        rf_struct("NAM0", "Bottom Left Coordinates", 8,
            sf_int32("X"),
            sf_int32("Y"),
        ),
        rf_struct("NAM9", "Top Right Coordinates", 8,
            sf_int32("X"),
            sf_int32("Y"),
        ),
        rf_formid("ZNAM", "Music"),
        rf_zstring("TNAM", "HD LOD Diffuse"),
        rf_zstring("UNAM", "HD LOD Normal"),
        rf_compressed("MHDT", "MHDT"),
        Field_MODL,
    ),
    .flags = record_flags(
        { 0x80000, "Can't Wait" },
    ),
};

static void clear_vtxt_junk(void* data, size_t size) {
    struct VTXT_Point {
        uint16_t position;
        uint8_t unk0;
        uint8_t unk1;
        float opacity;
    };
    static_assert(sizeof(VTXT_Point) == 8, "invalid Point size");

    auto now = (VTXT_Point*)data;
    auto end = (VTXT_Point*)((uint8_t*)data + size);
    verify((size % sizeof(VTXT_Point)) == 0);

    while (now < end) {
        // @TODO: to be honest I don't know is is junk or not.
        // Looks like junk though, it just randomly changes everytime you open CK and resave plugin.
        now->unk0 = 0;
        now->unk1 = 0;
        ++now;
    }
}

static RecordDef Record_LAND{
    .type = record_type("LAND"),
    .comment = "Landscape",
    .fields = record_fields(
        rf_bytes("VNML", "Vertex Normals"),
        rf_bytes("VHGT", "Vertex Height"),
        rf_compressed("VCLR", "Vertex Color"),
        rf_struct("BTXT", "Base Layer Header", 8,
            sf_formid("Land Texture"),
            sf_enum_uint8("Quadrant",
                { 0, "Bottom Left" },
                { 1, "Bottom Right" },
                { 2, "Upper Left" },
                { 3, "Upper Right" },
            ),
            sf_constant_array_with_fallback("Unknown", uint8_t, 3, 0x00, 0xff, 0xff),
        ),
        rf_struct("ATXT", "Alpha Layer Header", 8,
            sf_formid("Land Texture"),
            sf_enum_uint8("Quadrant",
                { 0, "Bottom Left" },
                { 1, "Bottom Right" },
                { 2, "Upper Left" },
                { 3, "Upper Right" },
            ),
            sf_constant_with_fallback("Unknown", &Type_uint8_t, uint8_t, 0),
            sf_uint16("Texture Layer"),
        ),
        rf_filter("VTXT", "Alpha Layer Data",
            &Type_ByteArrayRLE,
            clear_vtxt_junk
        ),
    ),
};

static RecordDef Record_LCTN{
    .type = record_type("LCTN"),
    .comment = "Location",
    .fields = record_fields(
        rf_formid("PNAM", "Parent Location"),
        rf_formid("MNAM", "Marker"),
        rf_float("RNAM", "World Location Radius"),
        rf_struct("CNAM", "Color", 4,
            sf_uint8("Red"),
            sf_uint8("Green"),
            sf_uint8("Blue"),
            sf_uint8("Alpha"),
        ),
    ),
};

static RecordDef Record_NAVM{
    .type = record_type("NAVM"),
    .comment = "NavMesh",
    .fields = record_fields(
        rf_bytes_rle("NVNM", "Geometry"),
    ),
};

static RecordDef Record_PACK{
    .type = record_type("PACK"),
    .comment = "Package",
    .fields = record_fields(
        rf_struct("PKDT", "Data", 12,
            sf_flags_uint32("Misc Flags",
                { 0x00000004, "Must Complete" },
                { 0x00000008, "Maintain Speed at Goal" },
                { 0x00000040, "At Package Start Unlock Doors" },
                { 0x00000080, "On Package Change Unlock Doors" },
                { 0x00000400, "Once Per Day" },
                { 0x00002000, "Preferred Speed" },
                { 0x00020000, "Always Sneak" },
                { 0x00040000, "Allow Swimming" },
                { 0x00100000, "Ignore Combat" },
                { 0x00200000, "Weapons Unequipped" },
                { 0x00800000, "Weapon Drawn" },
                { 0x08000000, "No Combat Alert" },
                { 0x20000000, "Wear Sleep Outfit" },
            ),
            sf_flags_uint8("Package Type",
                { 0x1, "Package Template" }
            ),
            sf_enum_uint8("Interrupt Override",
                { 0, "None" },
                { 4, "Combat" },
            ),
            sf_enum_uint8("Preferred Speed",
                { 0, "Walk" },
                { 1, "Jog" },
                { 2, "Run" },
                { 3, "Fast Walk" },
            ),
            sf_uint8("Unknown"),
            sf_flags_uint32("Interrupt Flags",
                { 0x001, "Hellos to Player" },
                { 0x002, "Random Conversations" },
                { 0x004, "Observe Combat Behavior" },
                { 0x008, "Greet Corpse Behavior" },
                { 0x010, "Reaction to Player Actions" },
                { 0x020, "Friendly Fire Comments" },
                { 0x040, "Aggro Radius Behavior" },
                { 0x080, "Allow Idle Chatter" },
                { 0x200, "World Interactions" },
            ),
        ),
        rf_struct("PSDT", "Schedule", 12,
            sf_constant(int8_t, -1),
            sf_int8("Day Of Week"),
            sf_int8("Date"),
            sf_int8("Hour"),
            sf_int8("Minute"),
            sf_uint8("Unknown"),
            sf_uint8("Unknown"),
            sf_uint8("Unknown"),
            sf_uint32("Duration"),
        ),
        rf_flags_uint8("IDLF", "Idle Flags",
            { 0x01, "Run In Sequence" },
            { 0x04, "Do Once" },
        ),
        rf_float("IDLT", "Idle Time"),
        rf_uint8("IDLC", "Idle Count"),
        rf_formid_array("IDLA", "Idle"),
        rf_struct("PKCU", "PKCU", 12,
            sf_uint32("Unknown"),
            sf_formid("Package Template"),
            sf_uint32("Unknown"),
        ),
        rf_formid("QNAM", "Owner Quest"),
    ),
};

static RecordDef Record_LCRT{
    .type = record_type("LCRT"),
    .comment = "Location Ref Type",
    .fields = record_fields(
        rf_struct("CNAM", "Color", 4,
            sf_uint8("Red"),
            sf_uint8("Green"),
            sf_uint8("Blue"),
            sf_constant(uint8_t, 0),
        ),
    ),
};

static RecordDef Record_ACTI{
    .type = record_type("ACTI"),
    .comment = "Activator",
    .fields = record_fields(
        rf_struct("PNAM", "Color", 4,
            sf_uint8("Red"),
            sf_uint8("Green"),
            sf_uint8("Blue"),
            sf_constant(uint8_t, 0),
        ),
        rf_uint16("FNAM", "Flags"),
    ),
};

static RecordDef Record_KEYM{
    .type = record_type("KEYM"),
    .comment = "Key",
    .fields = record_fields(
        Field_MODL,
        rf_formid("YNAM", "Pick Up Sound"),
        rf_formid("ZNAM", "Drop Sound"),
        rf_struct("DATA", "Data", 8,
            sf_uint32("Value"),
            sf_float("Weight"),
        ),
    ),
};

static RecordDef Record_BOOK{
    .type = record_type("BOOK"),
    .comment = "Book",
    .fields = record_fields(
        Field_MODL,
        rf_lstring("DESC", "Text"),
        rf_struct("DATA", "Data", 16, 
            sf_flags_uint8("Flags", 
                { 0x1, "Teaches Skill" },
                { 0x2, "Can't Be Taken" },
                { 0x4, "Teaches Spell" },
            ),
            sf_fixed_bytes<3>("Unknown"),
            sf_formid("Teaches"), // @TODO: uint32_t or FormID
            sf_uint32("Gold"),
            sf_float("Weight"),
        ),
        rf_formid("INAM", "Inventory Art"),
        rf_lstring("CNAM", "Description"),
    ),
};

static RecordDef Record_SCEN{
    .type = record_type("SCEN"),
    .comment = "Scene",
    .fields = record_fields(
        rf_uint32("WNAM", "Width"),
        rf_uint32("ALID", "Alias"),
        rf_int32("HTID", "Head Tracking Alias"),
        rf_float("DMAX", "Looping Max"),
        rf_float("DMIN", "Looping Min"),
        rf_uint32("DEMO", "Emotion"),
        rf_uint32("DEVA", "Emotion Value"),
    ),
};

RecordDef* get_record_def(RecordType type) {
    #define CASE(rec) case (RecordType)fourcc(#rec): return &Record_##rec
    switch (type) {
        CASE(TES4);
        CASE(WEAP);
        CASE(QUST);
        CASE(CELL);
        CASE(REFR);
        CASE(CONT);
        CASE(NPC_);
        CASE(NAVI);
        CASE(DLVW);
        CASE(DLBR);
        CASE(INFO);
        CASE(ACHR);
        CASE(DIAL);
        CASE(KYWD);
        CASE(TXST);
        CASE(GLOB);
        CASE(FACT);
        CASE(SOUN);
        CASE(MGEF);
        CASE(SPEL);
        CASE(FLST);
        CASE(STAT);
        CASE(MISC);
        CASE(FURN);
        CASE(WRLD);
        CASE(LAND);
        CASE(LCTN);
        CASE(NAVM);
        CASE(PACK);
        CASE(LCRT);
        CASE(ACTI);
        CASE(KEYM);
        CASE(BOOK);
        CASE(SCEN);
    }
    #undef CASE
    return nullptr;
}

const TypeEnumField* TypeEnum::get_field_by_value(uint32_t value) const {
    for (size_t i = 0; i < field_count; ++i) {
        const auto& field = fields[i];
        if (field.value == value) {
            return &field;
        }
    }
    return nullptr;
}
