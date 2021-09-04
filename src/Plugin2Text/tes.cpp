#include "tes.hpp"
#include "common.hpp"
#include "typeinfo.hpp"
#include "array.hpp"
#include <string.h>
#include <stdlib.h>

bool RawRecord::is_compressed() const {
    return is_bit_set(flags, RecordFlags::Compressed);
}

const char* record_group_type_to_string(RecordGroupType type) {
    switch (type) {
        case RecordGroupType::Top:                    return "Top";
        case RecordGroupType::WorldChildren:          return "World";
        case RecordGroupType::InteriorCellBlock:      return "Interior Block";
        case RecordGroupType::InteriorCellSubBlock:   return "Interior Sub-Block";
        case RecordGroupType::ExteriorCellBlock:      return "Exterior";
        case RecordGroupType::ExteriorCellSubBlock:   return "Exterior Sub-Block";
        case RecordGroupType::CellChildren:           return "Cell";
        case RecordGroupType::TopicChildren:          return "Topic";
        case RecordGroupType::CellPersistentChildren: return "Persistent";
        case RecordGroupType::CellTemporaryChildren:  return "Temporary";
        default: verify(false); return nullptr;
    }
}

static const char* const ShortMonthNames[12]{
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec",
};

const char* month_to_short_string(int month) {
    verify(month >= 1 && month <= 12);
    return ShortMonthNames[month - 1];
}

int short_string_to_month(const char* str) {
    auto count = strlen(str);
    verify(count == 3);
    for (int i = 0; i < _countof(ShortMonthNames); ++i) {
        if (memory_equals(ShortMonthNames[i], str, 3)) {
            return i + 1;
        }
    }
    verify(false);
    return 0; 
}

Array<VMAD_Script> VMAD_Field::parse_scripts(BinaryReader& r, uint16_t script_count, bool preserve_property_order) {
    Array<VMAD_Script> scripts{ tmpalloc };
    for (int i = 0; i < script_count; ++i) {
        VMAD_Script script;
        script.parse(r, this, preserve_property_order);
        scripts.push(script);
    }
    return scripts;
}

void VMAD_Field::parse(const uint8_t* value, size_t size, RecordType record_type, bool preserve_property_order) {
    BinaryReader r;
    r.start = value;
    r.now = r.start;
    r.end = r.start + size;

    const auto header = r.advance<VMAD_Header>();
    verify(header->version >= 2 && header->version <= 5);
    verify(header->object_format >= 1 && header->object_format <= 2);

    this->version = header->version;
    this->object_format = header->object_format;
    this->scripts = parse_scripts(r, header->script_count, preserve_property_order);
    this->contains_record_specific_info = r.now != r.end;

    if (contains_record_specific_info) {
        switch (record_type) {
            case RecordType::INFO: {
                verify(r.read<uint8_t>() == 2); // version?

                info.flags = r.read<PapyrusFragmentFlags>();
                info.script_name = r.advance_wstring();

                if (is_bit_set(info.flags, PapyrusFragmentFlags::HasBeginScript)) {
                    info.start_fragment.parse(r);
                }
                if (is_bit_set(info.flags, PapyrusFragmentFlags::HasEndScript)) {
                    info.end_fragment.parse(r);
                }
            } break;

            case RecordType::QUST: {
                verify(r.read<uint8_t>() == 2); // version?

                auto fragment_count = (int)r.read<uint16_t>();
                qust.file_name = r.advance_wstring();

                qust.fragments = Array<VMAD_QUST_Fragment>{ tmpalloc };
                for (int frag_index = 0; frag_index < fragment_count; ++frag_index) {
                    VMAD_QUST_Fragment fragment;
                    fragment.parse(r);
                    qust.fragments.push(fragment);
                };

                auto alias_count = (int)r.read<uint16_t>();
                qust.aliases = Array<VMAD_QUST_Alias>{ tmpalloc };
                for (int alias_index = 0; alias_index < alias_count; ++alias_index) {
                    VMAD_QUST_Alias alias;
                    
                    VMAD_ScriptPropertyValue value;
                    value.parse(r, this, PapyrusPropertyType::Object);
                    alias.object = value.as_object;
                    
                    verify(r.read<uint16_t>() == header->version);
                    verify(r.read<uint16_t>() == header->object_format);

                    const auto script_count = r.read<uint16_t>();
                    alias.scripts = parse_scripts(r, script_count, preserve_property_order);
                }
            } break;
        }
    }

    verify(r.now == r.end);
}

void VMAD_Script::parse(BinaryReader& r, const VMAD_Field* vmad, bool preserve_property_order) {
    name = r.advance_wstring();
    if (vmad->version >= 4) {
        status = r.read<uint8_t>();
    }

    auto property_count = r.read<uint16_t>();
    for (int prop_index = 0; prop_index < property_count; ++prop_index) {
        VMAD_ScriptProperty property;
        property.parse(r, vmad);
        properties.push(property);
    }

    if (!preserve_property_order) {
        qsort(properties.data, properties.count, sizeof(properties.data[0]), [](void const* aa, void const* bb) -> int {
            auto a = ((VMAD_ScriptProperty*)aa)->name;
            auto b = ((VMAD_ScriptProperty*)bb)->name;
            auto min_count = a->count > b->count ? b->count : a->count;
            int cmp = strncmp(a->data, b->data, min_count);
            return cmp == 0 ? a->count - b->count : cmp;
        });
    }
}

void VMAD_ScriptProperty::parse(BinaryReader& r, const VMAD_Field* vmad) {
    name = r.advance_wstring();
    type = r.read<PapyrusPropertyType>();
    if (vmad->version >= 4) {
        status = r.read<uint8_t>();
    }
    value.parse(r, vmad, type);
}

void VMAD_ScriptPropertyValue::parse(BinaryReader& r, const VMAD_Field* vmad, PapyrusPropertyType type) {
    verify(vmad->object_format == 2);
    switch (type) {
        case PapyrusPropertyType::Object: {
            auto value = r.read<VMAD_PropertyObjectV2>();
            as_object.form_id = value.form_id;
            as_object.alias = value.alias;
            verify(value.unused == 0);
        } break;

        case PapyrusPropertyType::String: {
            as_string = r.advance_wstring();
        } break;

        case PapyrusPropertyType::Int: {
            as_int = r.read<int>();
        } break;

        case PapyrusPropertyType::Float: {
            as_float = r.read<float>();
        } break;

        case PapyrusPropertyType::Bool: {
            as_bool = r.read<bool>();
        } break;

        case PapyrusPropertyType::ObjectArray:
        case PapyrusPropertyType::StringArray:
        case PapyrusPropertyType::IntArray:
        case PapyrusPropertyType::FloatArray:
        case PapyrusPropertyType::BoolArray: {
            const auto inner_type = (PapyrusPropertyType)((uint32_t)type - 10);
            as_array.count = r.read<uint32_t>();
            as_array.values = new VMAD_ScriptPropertyValue[as_array.count];
            for (uint32_t i = 0; i < as_array.count; ++i) {
                as_array.values[i].parse(r, vmad, inner_type);
            }
        } break;

        default: {
            verify(false);
        } break;
    }
}

void VMAD_INFO_Fragment::parse(BinaryReader& r) {
    verify(1 == r.read<uint8_t>());
    script_name = r.advance_wstring();
    fragment_name = r.advance_wstring();
}

void VMAD_QUST_Fragment::parse(BinaryReader& r) {
    index = r.read<uint16_t>();
    verify(0 == r.read<uint16_t>());
    log_entry = r.read<uint32_t>();
    verify(1 == r.read<uint8_t>());
    script_name = r.advance_wstring();
    function_name = r.advance_wstring();
}

void NVPP_Field::parse(const uint8_t* value, size_t size) {
    BinaryReader r{ value, size };

    const auto path_count = r.read<uint32_t>();
    grow(&paths, path_count);
    for (uint32_t i = 0; i < path_count; ++i) {
        NVPP_Path path;

        auto formid_count = r.read<uint32_t>();
        grow(&path.formids, formid_count); // @TODO: reserve instead of grow
        auto formids_start = r.advance(sizeof(FormID) * formid_count);
        memcpy(path.formids.data, formids_start, sizeof(FormID) * formid_count);
        path.formids.count = formid_count;
        
        paths.push(path);
    }

    const auto node_count = r.read<uint32_t>();
    grow(&nodes, node_count);
    for (uint32_t i = 0; i < node_count; ++i) {
        NVPP_Node node;
        node.formid.value = r.read<uint32_t>();
        node.index = r.read<uint32_t>();
        nodes.push(node);
    }

    qsort(nodes.data, nodes.count, sizeof(nodes.data[0]), [](void const* aa, void const* bb) -> int {
        NVPP_Node* a = (NVPP_Node*)aa;
        NVPP_Node* b = (NVPP_Node*)bb;

        return a->index - b->index;
    });

    verify(r.now == r.end);
}