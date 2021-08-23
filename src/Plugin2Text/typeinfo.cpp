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
    return { type, &Type_int8, name };
}

constexpr RecordFieldDef rf_int16(char const type[5], const char* name) {
    return { type, &Type_int16, name };
}

constexpr RecordFieldDef rf_int32(char const type[5], const char* name) {
    return { type, &Type_int32, name };
}

constexpr RecordFieldDef rf_int64(char const type[5], const char* name) {
    return { type, &Type_int64, name };
}

constexpr RecordFieldDef rf_uint8(char const type[5], const char* name) {
    return { type, &Type_uint8, name };
}

constexpr RecordFieldDef rf_uint16(char const type[5], const char* name) {
    return { type, &Type_uint16, name };
}

constexpr RecordFieldDef rf_uint32(char const type[5], const char* name) {
    return { type, &Type_uint32, name };
}

constexpr RecordFieldDef rf_uint64(char const type[5], const char* name) {
    return { type, &Type_uint64, name };
}

constexpr RecordFieldDef rf_formid(char const type[5], const char* name) {
    return { type, &Type_FormID, name };
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

#define sf_enum(m_name, m_size, ...)                    \
    ([]() -> const TypeEnum* {                          \
        static TypeEnumField fields[]{ __VA_ARGS__ };   \
        static TypeEnum type{ m_name, m_size, fields }; \
        return &type;                                   \
    })()

#define sf_enum_uint16(m_name, ...) sf_enum(m_name, 2, __VA_ARGS__)
#define sf_enum_uint32(m_name, ...) sf_enum(m_name, 4, __VA_ARGS__)

Type Type_ZString{ TypeKind::ZString, "CString", 0 };
Type Type_LString{ TypeKind::LString, "LString", 0 };
Type Type_ByteArray{ TypeKind::ByteArray, "Byte Array", 0 };
Type Type_ByteArrayCompressed{ TypeKind::ByteArrayCompressed, "Byte Array (Compressed)", 0 };
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
    static TypeEnum Type_##m_type{                       \
        m_name,                                          \
        m_size,                                          \
        CONCAT(Type_, m_type)_Fields                     \
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

//
//static RecordFieldDef Record_CELL_Fields[]{
    //{ "XCLL", &Type_CELL_XCLL, "Lighting" },
//};

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
        rf_int32("KSIZ", "Keyword Count"),
        { "KWDA", &Type_FormIDArray, "Keywords" },
        rf_zstring("FLTR", "Object Window Filter"),
    ),
    record_flags(
        { 0x20, "Deleted" },
        { (uint32_t)RecordFlags::Compressed, "Compressed" },
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
        rf_zstring("CNAM", "Author"),
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
      rf_uint16("DATA", "Flags"),
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
    )
);

RECORD(NPC_, "Non-Player Character",
    record_fields(
        rf_subrecord("ACBS", "Base Stats", 24,
            sf_uint32("Flags"), // @TODO: wrong stuff
            sf_uint16("Magicka Offset"),
            sf_uint16("Stamina Offset"),
            sf_uint16("Level"),
            sf_uint16("Calc Min Level"),
            sf_uint16("Calc Max Level"),
            sf_uint16("Speed Multiplier"),
            sf_uint16("Disposition Base"),
            sf_uint16("Template Data Flags"),
            sf_uint16("Health Offset"),
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
            { "Loud", 0 },
            { "Normal", 1 },
            { "Silent", 2 },
            { "Very Loud", 3 },
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
        rf_subrecord("QNAM", "Skin Tone", 12, 
            sf_float("Red"),
            sf_float("Green"),
            sf_float("Blue"),
        ),
    ),
);

RECORD(NAVI, "Navigation",
    record_fields(
        rf_uint32("NVER", "Version"),
        rf_bytes("NVMI", "Navmesh Data"),
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
        rf_zstring("NAM2", "Notes"),
        rf_zstring("NAM3", "Edits"),
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
    )
);

RECORD(DIAL, "Dialogue Topic",
    record_fields(
        rf_float("PNAM", "Priority"),
        rf_formid("BNAM", "Owning Branch"),
        rf_formid("QNAM", "Owning Quest"),
        rf_uint32("TIFC", "Info Count"),
    )
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
            { "not has specular map", 0x01 },
            { "facegen textures", 0x02 },
            { "has model space normal map", 0x04 },
        ),
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
    }
    #undef CASE
    return nullptr;
}