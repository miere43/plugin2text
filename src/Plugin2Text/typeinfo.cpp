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

constexpr RecordFieldDef rf_int8(char const type[5], const char* name) {
    return { type, &Type_int8_t, name };
}

constexpr RecordFieldDef rf_int16(char const type[5], const char* name) {
    return { type, &Type_int16_t, name };
}

constexpr RecordFieldDef rf_int32(char const type[5], const char* name) {
    return { type, &Type_int32_t, name };
}

constexpr RecordFieldDef rf_int64(char const type[5], const char* name) {
    return { type, &Type_int64_t, name };
}

constexpr RecordFieldDef rf_uint8(char const type[5], const char* name) {
    return { type, &Type_uint8_t, name };
}

constexpr RecordFieldDef rf_uint16(char const type[5], const char* name) {
    return { type, &Type_uint16_t, name };
}

constexpr RecordFieldDef rf_uint32(char const type[5], const char* name) {
    return { type, &Type_uint32_t, name };
}

constexpr RecordFieldDef rf_uint64(char const type[5], const char* name) {
    return { type, &Type_uint64_t, name };
}

constexpr RecordFieldDef rf_formid(char const type[5], const char* name) {
    return { type, &Type_FormID, name };
}

constexpr RecordFieldDef rf_formid_array(char const type[5], const char* name) {
    return { type, &Type_FormIDArray, name };
}

constexpr RecordFieldDef rf_bytes(char const type[5], const char* name) {
    return { type, &Type_ByteArray, name };
}

constexpr RecordFieldDef rf_compressed(char const type[5], const char* name) {
    return { type, &Type_ByteArrayCompressed, name };
}

#define rf_subrecord_shared(m_record, m_subrecord, m_name) \
    { #m_subrecord, &CONCAT(Type_, m_record)_##m_subrecord, m_name }

#define rf_subrecord(m_subrecord, m_name, m_size, ...)    \
    ([]() -> RecordFieldDef {                             \
        static TypeStructField fields[]{ __VA_ARGS__ };   \
        static TypeStruct type{ m_name, m_size, fields }; \
        return { m_subrecord, &type, m_name };            \
    })()

#define rf_enum(m_subrecord, m_name, m_size, m_flags, ...)       \
    ([]() -> RecordFieldDef {                                    \
        static TypeEnumField fields[]{ __VA_ARGS__ };            \
        static TypeEnum type{ m_name, m_size, fields, m_flags }; \
        return { m_subrecord, &type, m_name };                   \
    })()

#define rf_enum_uint8(m_subrecord, m_name, ...) rf_enum(m_subrecord, m_name, 1, false, __VA_ARGS__)
#define rf_enum_uint16(m_subrecord, m_name, ...) rf_enum(m_subrecord, m_name, 2, false, __VA_ARGS__)
#define rf_enum_uint32(m_subrecord, m_name, ...) rf_enum(m_subrecord, m_name, 4, false, __VA_ARGS__)

#define rf_flags_uint8(m_subrecord, m_name, ...) rf_enum(m_subrecord, m_name, 1, true, __VA_ARGS__)
#define rf_flags_uint16(m_subrecord, m_name, ...) rf_enum(m_subrecord, m_name, 2, true, __VA_ARGS__)
#define rf_flags_uint32(m_subrecord, m_name, ...) rf_enum(m_subrecord, m_name, 4, true, __VA_ARGS__)

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

template<typename T>
constexpr TypeStructField sf_constant(const T& value) {
    static T the_value = value;
    static TypeConstant constant{ "Constant", sizeof(T), (const uint8_t*)&the_value };
    return { &constant, "Constant" };
}

template<size_t N>
constexpr TypeStructField sf_fixed_bytes(const char* name) {
    static Type Type_ByteArrayFixed{ TypeKind::ByteArrayFixed, name, N };
    return { &Type_ByteArrayFixed, name };
}

#define sf_enum(m_name, m_size, m_flags, ...)                    \
    ([]() -> TypeStructField {                                   \
        static TypeEnumField fields[]{ __VA_ARGS__ };            \
        static TypeEnum type{ m_name, m_size, fields, m_flags }; \
        return { &type, m_name };                                \
    })()

#define sf_enum_uint8(m_name, ...) sf_enum(m_name, 1, false, __VA_ARGS__)
#define sf_enum_uint16(m_name, ...) sf_enum(m_name, 2, false, __VA_ARGS__)
#define sf_enum_uint32(m_name, ...) sf_enum(m_name, 4, false, __VA_ARGS__)

#define sf_flags_uint8(m_name, ...) sf_enum(m_name, 1, true, __VA_ARGS__)
#define sf_flags_uint16(m_name, ...) sf_enum(m_name, 2, true, __VA_ARGS__)
#define sf_flags_uint32(m_name, ...) sf_enum(m_name, 4, true, __VA_ARGS__)

Type Type_ZString{ TypeKind::ZString, "CString", 0 };
Type Type_LString{ TypeKind::LString, "LString", 0 };
Type Type_WString{ TypeKind::WString, "WString", 0 };
Type Type_ByteArray{ TypeKind::ByteArray, "Byte Array", 0 };
Type Type_ByteArrayCompressed{ TypeKind::ByteArrayCompressed, "Byte Array (Compressed)", 0 };
Type Type_float{ TypeKind::Float, "float", sizeof(float) };
Type Type_FormID{ TypeKind::FormID, "Form ID", sizeof(int) };
Type Type_FormIDArray{ TypeKind::FormIDArray, "Form ID Array", 0 };
Type Type_bool{ TypeKind::Boolean, "bool", sizeof(bool) };
Type Type_VMAD{ TypeKind::VMAD, "VMAD", 0 };
TypeInteger Type_int8_t{ "int8", sizeof(int8_t), false };
TypeInteger Type_int16_t{ "int16", sizeof(int16_t), false };
TypeInteger Type_int32_t{ "int32", sizeof(int32_t), false };
TypeInteger Type_int64_t{ "int64", sizeof(int64_t), false };
TypeInteger Type_uint8_t{ "uint8", sizeof(uint8_t), true };
TypeInteger Type_uint16_t{ "uint16", sizeof(uint16_t), true };
TypeInteger Type_uint32_t{ "uint32", sizeof(uint32_t), true };
TypeInteger Type_uint64_t{ "uint64", sizeof(uint64_t), true };

TypeStructField Type_Vector3_Fields[] = {
    sf_float("X"),
    sf_float("Y"),
    sf_float("Z"),
};

TypeStruct Type_Vector3{ "Vector3", 12, Type_Vector3_Fields };

auto Field_MODL = rf_zstring("MODL", "Model File Name");

const RecordFieldDef* RecordDef::get_field_def(RecordFieldType type) const {
    for (int i = 0; i < fields.count; ++i) {
        const auto& field = fields.data[i];
        if (field.type == type) {
            return &field;
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

TYPE_STRUCT(CNTO, "Item", 8,
    sf_formid("Item"),
    sf_uint32("Count"),
);

TYPE_ENUM(PapyrusPropertyType, "Papyrus Property Type", 1,
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

#define RECORD(m_type, m_name, ...)  \
    static RecordDef Record_##m_type{ \
        #m_type,                      \
        m_name,                       \
        __VA_ARGS__                   \
    }

#define record_flags(...)                             \
    ([]() -> StaticArray<RecordFlagDef> {             \
        static RecordFlagDef fields[]{ __VA_ARGS__ }; \
        return { fields, _countof(fields) };          \
    })()

#define record_fields(...)                             \
    ([]() -> StaticArray<RecordFieldDef> {             \
        static RecordFieldDef fields[]{ __VA_ARGS__ }; \
        return { fields, _countof(fields) };           \
    })()

RecordDef Record_Common{
    "0000",
    "-- common --",
    record_fields(
        rf_zstring("EDID", "Editor ID"),
        rf_lstring("FULL", "Name"),
        rf_subrecord("OBND", "Object Bounds", 12,
            sf_int16("X1"),
            sf_int16("Y1"),
            sf_int16("Z1"),
            sf_int16("X2"),
            sf_int16("Y2"),
            sf_int16("Z2"),
        ),
        rf_int32("COCT", "Item Count"),
        { "CNTO", &Type_CNTO, "Items" },
        { "VMAD", &Type_VMAD, "Script" },
        rf_int32("KSIZ", "Keyword Count"),
        rf_formid_array("KWDA", "Keywords"),
        rf_zstring("FLTR", "Object Window Filter"),
    ),
    record_flags(
        { 0x20, "Deleted" },
        { (uint32_t)RecordFlags::Compressed, "Compressed" },
        { 0x800000, "Is Marker" },
        { 0x8000000, "NavMesh Generation - Bounding Box" },
    ),
};

RECORD(
    TES4,
    "File Header",
    record_fields(
        rf_subrecord("HEDR", "Header", 12,
            sf_float("Version"),
            sf_int32("Number Of Records"),
            sf_int32("Next Object ID"), 
        ),
        rf_zstring("MAST", "Master File"),
        rf_uint64("DATA", "Unused"),
        rf_zstring("CNAM", "Author"),
        rf_uint32("INTV", "Tagified Strings"),
        //rf_zstring("SNAM", "Description"), // @TODO: Multiline strings!
    ),
    record_flags(
        { 0x1, "Master" },
        { (uint32_t)RecordFlags::TES4_Localized, "Localized" },
        { 0x200, "Light Master" },
    )
);

RECORD(WEAP, "Weapon",
    record_fields(
        rf_formid("ETYP", "Equipment Type"),
        rf_formid("BIDS", "Block Bash Impact Data Set"),
        rf_formid("BAMT", "Alternate Block Material"),
        rf_lstring("DESC", "Description"),
        rf_formid("INAM", "Impact Data Set"),
        rf_formid("WNAM", "1st Person Model Object"),
        rf_formid("TNAM", "Attack Fail Sound"),
        rf_formid("NAM9", "Equip Sound"),
        rf_formid("NAM8", "Unequip Sound"),
        rf_subrecord("DATA", "Game Data", 10,
            sf_int32("Value"),
            sf_float("Weight"),
            sf_int16("Damage"), 
        ),
        rf_subrecord("DNAM", "Weapon Data", 100,
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
        ),
        rf_subrecord("CRDT", "Critical Data", 24,
            sf_uint16("Critical Damage"),
            sf_uint16("Unknown"),
            sf_float("Critical % Mult"),
            sf_uint32("Flags"),
            sf_uint32("Unknown"),
            sf_formid("Critical Spell Effect"),
            sf_uint32("Unknown"),
        ),
        rf_int32("VNAM", "Detection Sound Level"),
    )
);

RECORD(QUST, "Quest",
    record_fields(
        rf_subrecord("DNAM", "Quest Data", 12,
            sf_uint8("Flags"),
            sf_uint8("Flags 2"),
            sf_uint8("Priority"),
            sf_uint8("Unknown"),
            sf_int32("Unknown 2"),
            sf_uint32("Quest Type"),
        ),
        rf_subrecord("INDX", "Index", 4, 
            sf_uint16("Journal Index"),
            sf_uint8("Flags"),
            sf_int8("Unknown"),
        ),
        rf_lstring("CNAM", "Journal Entry"),
        rf_uint8("QSDT", "Flags"),
        rf_int16("QOBJ", "Objective Index"),
        rf_uint32("FNAM", "Objective Flags"),
        rf_lstring("NNAM", "Objective Text"),
        rf_subrecord("QSTA", "Quest Target", 8,
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
    )
);

RECORD(CELL, "Cell",
    record_fields(
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
        rf_subrecord("XCLC", "Data", 12,
            sf_int32("X"),
            sf_int32("Y"),
            sf_flags_uint32("Flags",
                { 0x1, "Force Hide Land Quad 1" },
                { 0x2, "Force Hide Land Quad 2" },
                { 0x4, "Force Hide Land Quad 3" },
                { 0x8, "Force Hide Land Quad 4" },
            ),
        ),
        rf_formid("LTMP", "Lighting Template"),
        rf_formid_array("XCLR", "Regions Containing Cell"),
        rf_formid("XLCN", "Location"),
        rf_formid("XCWT", "Water"),
        // XCLW
    ),
    record_flags(
        { 0x400, "Persistent" },
    ),
);

auto Type_LocationData = rf_subrecord("DATA", "Data", 24,
    sf_float("Pos X"),
    sf_float("Pos Y"),
    sf_float("Pos Z"),
    sf_float("Rot X"),
    sf_float("Rot Y"),
    sf_float("Rot Z"),
);

RECORD(REFR, "Reference",
    record_fields(
        rf_formid("NAME", "Base Form ID"),
        rf_float("XSCL", "Scale"),
        rf_flags_uint8("XAPD", "Activation Parent Flags",
            { 0x1, "Parent Activate Only" },
        ),
        rf_subrecord("XAPR", "Activation Parent", 8,
            sf_formid("Form ID"),
            sf_float("Delay"),
        ),
        rf_flags_uint8("FNAM", "Marker Flags",
            { 0x1, "Visible" },
            { 0x2, "Can Travel To" },
            { 0x4, "Show All" },
        ),
        rf_subrecord("XNDP", "Door Pivot", 8,
            sf_formid("NavMesh"),
            sf_uint16("NavMesh Triangle Index"),
            sf_uint16("Unknown"), // sf_constant<uint16_t>(0),
        ),
        rf_subrecord("XLKR", "Linked Reference", 8,
            sf_formid("Keyword"),
            sf_formid("Reference"),
        ),
        rf_subrecord("XTEL", "Door Teleport", 32,
            sf_formid("Destination Door"),
            sf_float("Pos X"), // @TODO: Vector3 type
            sf_float("Pos Y"),
            sf_float("Pos Z"),
            sf_float("Rot X"),
            sf_float("Rot Y"),
            sf_float("Rot Z"),
            sf_flags_uint32("Flags",
                { 0x1, "No Alarm" },
            ),
        ),
        rf_formid("XLRL", "Location"),
        Type_LocationData,
    ),
    record_flags(
        { 0x400, "Persistent" },
    )
);

RECORD(CONT, "Container",
    record_fields(
        rf_subrecord("DATA", "Data", 5,
            sf_uint8("Flags"),
            sf_float("Unknown"), 
        ),
        Field_MODL,
    )
);

RECORD(NPC_, "Non-Player Character",
    record_fields(
        rf_subrecord("ACBS", "Base Stats", 24,
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
        rf_subrecord("NAM9", "Face Morph", 76,
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
        rf_subrecord("PRKR", "Perk", 8, 
            sf_formid("Perk"),
            sf_uint32("Unknown"), 
        ),
        rf_subrecord("AIDT", "AI Data", 20,
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
        rf_subrecord("DNAM", "Data", 52,
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
            sf_uint8("Mod Skill - One-Handed"),
            sf_uint8("Mod Skill - Two-Handed"),
            sf_uint8("Mod Skill - Marksman"),
            sf_uint8("Mod Skill - Block"),
            sf_uint8("Mod Skill - Smithing"),
            sf_uint8("Mod Skill - Heavy Armor"),
            sf_uint8("Mod Skill - Light Armor"),
            sf_uint8("Mod Skill - Pickpocket"),
            sf_uint8("Mod Skill - Lockpicking"),
            sf_uint8("Mod Skill - Sneak"),
            sf_uint8("Mod Skill - Alchemy"),
            sf_uint8("Mod Skill - Speechcraft"),
            sf_uint8("Mod Skill - Alteration"),
            sf_uint8("Mod Skill - Conjuration"),
            sf_uint8("Mod Skill - Destruction"),
            sf_uint8("Mod Skill - Illusion"),
            sf_uint8("Mod Skill - Restoration"),
            sf_uint8("Mod Skill - Enchanting"),
            sf_uint16("Calculated Health"),
            sf_uint16("Calculated Magicka"),
            sf_uint16("Calculated Stamina"),
            sf_uint16("Unknown"),
            sf_float("Far Away Model Distance"),
            sf_uint8("Geared Up Weapons"),
            sf_fixed_bytes<3>("Unknown"),
        ),
        rf_subrecord("QNAM", "Skin Tone", 12, 
            sf_float("Red"),
            sf_float("Green"),
            sf_float("Blue"),
        ),
        rf_subrecord("NAMA", "Face Parts", 16,
            sf_int32("Nose"),
            sf_int32("Unknown"),
            sf_int32("Eyes"),
            sf_int32("Mouth"),
        ),
        rf_uint16("TINI", "Tint Item"),
        rf_subrecord("TINC", "Tint Color", 4,
            sf_uint8("Red"),
            sf_uint8("Green"),
            sf_uint8("Blue"),
            sf_uint8("Alpha"), // this is 0 or 255
        ),
        rf_int32("TINV", "Tint Value"),
    ),
);

RECORD(NAVI, "Navigation",
    record_fields(
        rf_uint32("NVER", "Version"),
        rf_bytes("NVMI", "NavMesh Data"),
        rf_compressed("NVPP", "Preferred Pathing Data"),
    ),
);

RECORD(DLVW, "Dialogue View",
    record_fields(
        rf_formid("QNAM", "Parent Quest"),
        rf_formid("BNAM", "Branch"),
        rf_formid("TNAM", "Topic"),
        rf_uint32("ENAM", "Unknown"),
        rf_uint8("DNAM", "Show All Text"),
    ),
);

RECORD(DLBR, "Dialogue Branch",
    record_fields(
        rf_formid("QNAM", "Parent Quest"),
        rf_uint32("TNAM", "Unknown"),
        rf_uint32("DNAM", "Flags"),
        rf_formid("SNAM", "Start Dialogue"),
    ),
);

RECORD(INFO, "Topic Info",
    record_fields(
        rf_subrecord("ENAM", "Data", 4,
            sf_uint16("Flags"),
            sf_uint16("Reset Time"),
        ),
        rf_formid("PNAM", "Previous Info"),
        rf_uint8("CNAM", "Favor Level"),
        rf_formid("TCLT", "Topic Links"),
        rf_lstring("NAM1", "Response"), // @TODO: ilstring
        rf_zstring("NAM2", "Notes"),
        rf_zstring("NAM3", "Edits"),
        rf_lstring("RNAM", "Player Response"),
        rf_subrecord("TRDT", "Response", 24,
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
            sf_uint32("Unknown"), // constant 0
            sf_uint8("Response Index"),
            sf_fixed_bytes<3>("Unknown"),
            sf_formid("Sound"),
            sf_bool("Use Emotion Animation"),
            sf_fixed_bytes<3>("Unknown"),
        ),
    ),
);

RECORD(ACHR, "Actor",
    record_fields(
        rf_formid("NAME", "Base NPC"),
        rf_bytes("XRGD", "Ragdoll Data"),
        Type_LocationData,
    ),
    record_flags(
        { 0x200, "Starts Dead" },
    ),
);

RECORD(DIAL, "Dialogue Topic",
    record_fields(
        rf_float("PNAM", "Priority"),
        rf_formid("BNAM", "Owning Branch"),
        rf_formid("QNAM", "Owning Quest"),
        rf_uint32("TIFC", "Info Count"),
        // @TODO: SNAM = char[4]
    ),
);

RECORD(KYWD, "Keyword",
    record_fields(
        rf_subrecord("CNAM", "Color", 4,
            sf_uint8("Red"),
            sf_uint8("Green"),
            sf_uint8("Blue"),
            sf_uint8("Alpha"), // this is constant 0
        ),
    ),
);

RECORD(TXST, "Texture Set",
    record_fields(
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
);

RECORD(GLOB, "Global",
    record_fields(
        rf_enum_uint8("FNAM", "Type",
            { 's', "Short" },
            { 'l', "Long" },
            { 'f', "Float" },
        ),
        rf_float("FLTV", "Value"),
    ),
);

RECORD(FACT, "Faction",
    record_fields(
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
);

RECORD(SOUN, "Sound",
    record_fields(
        rf_formid("SDSC", "Sound Descriptor"),
    ),
);

RECORD(MGEF, "Magic Effect",
    record_fields(
        rf_zstring("DNAM", "Description"),
    ),
);

RECORD(SPEL, "Spell",
    record_fields(
        rf_formid("ETYP", "Equipment Type"),
        rf_zstring("DESC", "Description"),
        rf_formid("EFID", "Magic Effect Form ID"),
        rf_subrecord("EFIT", "Magic Effect", 12,
            sf_float("Magnitude"),
            sf_uint32("Area of Effect"),
            sf_uint32("Duration"),
        ),
    ),
);

RECORD(FLST, "Form List",
    record_fields(
        rf_formid("LNAM", "Object"),
    ),
);

RECORD(STAT, "Static",
    record_fields(
        rf_subrecord("DNAM", "Data", 12, 
            sf_float("Max Angle"),
            sf_formid("Directional Material"),
            sf_uint32("Unknown"),
        ),
        Field_MODL,
    ),
);

RECORD(MISC, "Misc Item",
    record_fields(
        Field_MODL,
    ),
);

RECORD(FURN, "Furniture",
    record_fields(
        Field_MODL,
        rf_zstring("XMRK", "Marker Model File Name"),
    ),
);

RECORD(WRLD, "Worldspace",
    record_fields(
        rf_formid("CNAM", "Climate"),
        rf_formid("NAM2", "Water"),
        rf_formid("NAM3", "LOD Water Type"),
        rf_float("NAM4", "LOD Water Height"),
        rf_subrecord("DNAM", "Land Data", 8,
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
        rf_subrecord("NAM0", "Bottom Left Coordinates", 8,
            sf_int32("X"),
            sf_int32("Y"),
        ),
        rf_subrecord("NAM9", "Top Right Coordinates", 8,
            sf_int32("X"),
            sf_int32("Y"),
        ),
        rf_formid("ZNAM", "Music"),
        rf_zstring("TNAM", "HD LOD Diffuse"),
        rf_zstring("UNAM", "HD LOD Normal"),
    ),
    record_flags(
        { 0x80000, "Can't Wait" },
    ),
);

RECORD(LAND, "Landscape",
    record_fields(
        rf_bytes("VNML", "Vertex Normals"),
        rf_bytes("VHGT", "Vertex Height"),
        rf_bytes("VCLR", "Vertex Color"),
        rf_subrecord("BTXT", "Base Texture", 8,
            sf_formid("Land Texture"),
            sf_enum_uint8("Quadrant", 
                { 0, "Bottom Left" },
                { 1, "Bottom Right" },
                { 2, "Upper Left" },
                { 3, "Upper Right" },
            ),
            sf_fixed_bytes<3>("Unknown"),
        ),
        rf_subrecord("ATXT", "Additional Texture", 8,
            sf_formid("Land Texture"),
            sf_enum_uint8("Quadrant",
                { 0, "Bottom Left" },
                { 1, "Bottom Right" },
                { 2, "Upper Left" },
                { 3, "Upper Right" },
            ),
            sf_uint8("Unknown"),
            sf_uint16("Texture Layer"),
        ),
    ),
);

RECORD(LCTN, "Location",
    record_fields(
        rf_formid("PNAM", "Parent Location"),
        rf_formid("MNAM", "Marker"),
        rf_float("RNAM", "World Location Radius"),
        rf_subrecord("CNAM", "Color", 4,
            sf_uint8("Red"),
            sf_uint8("Green"),
            sf_uint8("Blue"),
            sf_uint8("Alpha"),
        ),
    ),
);

RECORD(NAVM, "NavMesh",
    record_fields(
        rf_bytes("NVNM", "Geometry"),
    ),
);

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
    }
    #undef CASE
    return nullptr;
}