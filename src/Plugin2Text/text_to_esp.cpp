#include "text_to_esp.hpp"
#include "os.hpp"
#include "common.hpp"
#include <stdio.h>
#include <stdlib.h>
#include "base64.hpp"
#include <zlib.h>
#include <charconv>

void TextRecordReader::init() {
    esp_buffer = allocate_virtual_memory(1024 * 1024 * 1024);
    compression_buffer = allocate_virtual_memory(1024 * 1024 * 32);
    buffer = &esp_buffer;
}

void TextRecordReader::dispose() {
    free_virtual_memory(&esp_buffer);
    free_virtual_memory(&compression_buffer);
    *this = TextRecordReader();
}

void TextRecordReader::read_records(const char* start, const char* end) {
    this->start = start;
    this->now = start;
    this->end = end;

    verify(expect("plugin2text version 1.00\n---\n"));

    while (now < end) {
        read_record();
    }
}

RecordGroupType TextRecordReader::expect_record_group_type() {
    #define CASE(m_type, m_string) if (expect(m_string)) return RecordGroupType::m_type
    CASE(WorldChildren, "World");
    CASE(InteriorCellSubBlock, "Interior Sub-Block");
    CASE(InteriorCellBlock, "Interior Block");
    CASE(ExteriorCellSubBlock, "Exterior Sub-Block");
    CASE(ExteriorCellBlock, "Exterior");
    CASE(CellChildren, "Cell");
    CASE(TopicChildren, "Topic");
    CASE(Top, "Top");
    CASE(CellPersistentChildren, "Persistent");
    CASE(CellTemporaryChildren, "Temporary");
    #undef CASE
    verify(false);
    return (RecordGroupType)0;
}

uint16_t TextRecordReader::read_record_timestamp() {
    // @TODO: ugly
    auto current_indent = peek_indents();
    if (current_indent == indent + 1) {
        auto line_end = peek_end_of_current_line();
        auto curr = &now[current_indent * 2];
        if (curr + 1 < line_end && curr[0] >= '1' && curr[0] <= '9') {
            int d = 0, y = 0;
            char month_name[4];
            verify(3 == _snscanf_s(curr, line_end - curr, "%d %3s 20%d", &d, month_name, (int)_countof(month_name), &y));
            now = line_end + 1; // skip \n
            int m = short_string_to_month(month_name);
            return
                ((y << 9) & 0b1111111'0000'00000) |
                ((m << 5) & 0b0000000'1111'00000) |
                ((d << 0) & 0b0000000'0000'11111);
        }
    }
    return 0;
}

uint32_t TextRecordReader::read_record_unknown() {
    // @TODO: ugly
    auto current_indent = peek_indents();
    if (current_indent == indent + 1) {
        auto line_end = peek_end_of_current_line();
        auto curr = &now[current_indent * 2];

        constexpr const char* Unknown = "Unknown = ";
        constexpr size_t UnknownCount = sizeof("Unknown = ") - 1;

        if (curr + UnknownCount <= line_end && memory_equals("Unknown = ", curr, UnknownCount)) {
            uint32_t value;
            curr += UnknownCount;
            verify(1 == _snscanf_s(curr, line_end - curr, "%X", &value));
            now = line_end + 1; // skip \n
            return value;
        }
    }
    return 0;
}

void TextRecordReader::read_grup_record(RawGrupRecord* group) {
    if (expect(" - ")) {
        auto line_end = peek_end_of_current_line();
        group->group_type = expect_record_group_type();

        switch (group->group_type) {
            case RecordGroupType::InteriorCellBlock:
            case RecordGroupType::InteriorCellSubBlock: {
                verify(expect(" "));
                group->label = expect_int();
            } break;
                
            case RecordGroupType::ExteriorCellBlock:
            case RecordGroupType::ExteriorCellSubBlock: {
                verify(expect(" ("));
                group->grid_x = expect_int();
                verify(expect("; "));
                group->grid_y = expect_int();
                verify(expect(")"));
            } break;

            case RecordGroupType::CellChildren: 
            case RecordGroupType::CellPersistentChildren: 
            case RecordGroupType::CellTemporaryChildren: 
            case RecordGroupType::WorldChildren:
            case RecordGroupType::TopicChildren: {
                // @TODO: It's possible to infer "group->value" by looking at previous record.
                verify(expect(" "));
                group->label = read_formid().value;
            } break;

            default: {
                verify(false);
            } break;
        }
    } else {
        group->group_type = RecordGroupType::Top;
    }

    skip_to_next_line();

    group->timestamp = read_record_timestamp();
    group->unknown = read_record_unknown();

    ++indent;
    while (try_continue_current_indent()) {
        auto record = read_record();
        if (group->group_type == RecordGroupType::Top) {
            if (record->type == RecordType::GRUP) {
                if (!group->label) {
                    auto record_as_group = (RawGrupRecord*)record;
                    switch (record_as_group->group_type) {
                        case RecordGroupType::InteriorCellBlock: {
                            group->label = fourcc("CELL");
                        } break;

                        case RecordGroupType::WorldChildren: {
                            group->label = fourcc("WRLD");
                        } break;

                        default: {
                            verify(false);
                        } break;
                    }
                }
            } else if (group->label == 0) {
                group->label = (uint32_t)record->type;
            } else {
                verify(group->label == (uint32_t)record->type);
            }
        }
    }
    --indent;

    // validate
    switch (group->group_type) {
        case RecordGroupType::Top: {
            verify(group->label);
        } break;

        // @TODO: validate other group_type's
    }

    group->group_size = static_cast<uint32_t>(buffer->now - reinterpret_cast<const uint8_t*>(group));
}

RecordFlags TextRecordReader::read_record_flags(RecordDef* def) {
    RecordFlags flags = RecordFlags::None;

    int def_count;
    RecordDef* defs[2];
    if (def) {
        def_count = 2;
        defs[0] = def;
    } else {
        def_count = 1;
    }
    defs[def_count - 1] = &Record_Common;

    ++indent;
    while (try_continue_current_indent()) {
        if (&now[indent * 2] < end && now[indent * 2] != '+') {
            break;
        }
        expect_indent();
        verify(expect("+ "));

        auto line_end = peek_end_of_current_line();
        auto count = line_end - now;
        verify(count > 0);

        // bypass flag checks if this is HEX number (we can have FFFFFFFF but whatever).
        if (!(now[0] >= '0' && now[0] <= '9')) {
            for (int def_index = 0; def_index < def_count; ++def_index) {
                const auto current_def = defs[def_index];
                for (int i = 0; i < current_def->flags.count; ++i) {
                    const auto& flag = current_def->flags.data[i];
                    if (strlen(flag.name) == count && memory_equals(flag.name, now, count)) {
                        flags |= (RecordFlags)flag.bit;
                        goto ok;
                    }
                }
            }
        }

        uint32_t unrecognized_flags;
        int nread;
        verify(1 == _snscanf_s(now, count, "%X%n", &unrecognized_flags, &nread));
        verify(nread == count);

        flags |= (RecordFlags)unrecognized_flags;

        ok: {}
        now = line_end + 1; // skip \n
    }
    --indent;

    return flags;
}

RawRecord* TextRecordReader::read_record() {
    expect_indent();
    
    static_assert(sizeof(RawRecord) == sizeof(RawGrupRecord), "invalid raw record sizes");

    const auto record = buffer->advance<RawRecord>();
    record->type = read_record_type();
    current_record_type = record->type;

    if (record->type == RecordType::GRUP) {
        read_grup_record((RawGrupRecord*)record);
        return record;
    }

    verify(expect(" "));
    record->id = read_formid();

    {
        const auto line_end = peek_end_of_current_line();
        if (expect(",v")) {
            int version = 0;
            verify(1 == _snscanf_s(now, line_end - now, "%d", &version));
            verify(version >= 1 && version <= 44);
            record->version = static_cast<uint16_t>(version);
        } else {
            record->version = 44;
        }
        now = line_end + 1; // skip \n
    }

    record->timestamp = read_record_timestamp();
    {
        auto unknown = read_record_unknown();
        verify(unknown <= 0xffff);
        record->unknown = static_cast<uint16_t>(unknown);
    }

    auto def = get_record_def(record->type);
    record->flags = read_record_flags(def);
    if (!def) {
        def = &Record_Common;
    }

    const bool use_compression_buffer = record->is_compressed();
    if (use_compression_buffer) {
        verify(!inside_compressed_record);
        inside_compressed_record = true;
        buffer = &compression_buffer;
    }

    ++indent;
    while (try_continue_current_indent()) {
        const auto curr = &now[indent * 2];
        verify(curr + sizeof(RecordFieldType) <= end);
        const auto field_type = *(RecordFieldType*)curr;

        auto field_def = def->get_field_def(field_type);
        if (!field_def) {
            field_def = Record_Common.get_field_def(field_type);
        }

        if (!field_def || field_def->def_type == RecordFieldDefType::Field) {
            read_field(static_cast<const RecordFieldDef*>(field_def));
        } else if (field_def->def_type == RecordFieldDefType::Subrecord) {
            read_subrecord_fields(static_cast<const RecordFieldDefSubrecord*>(field_def));
        } else {
            verify(false);
        }
    }
    --indent;

    if (use_compression_buffer) {
        constexpr int SkyrimZLibCompressionLevel = 7;

        auto uncompressed_data_size = static_cast<uLong>(compression_buffer.now - compression_buffer.start);
        verify(uncompressed_data_size > 0);

        const auto record_compressed = (RawRecordCompressed*)record;

        auto compressed_size = static_cast<uLongf>(esp_buffer.end - esp_buffer.now); // remaining ESP size
        const auto result = ::compress2((uint8_t*)(record_compressed + 1), &compressed_size, buffer->start, uncompressed_data_size, SkyrimZLibCompressionLevel);
        verify(result == Z_OK);

        record_compressed->uncompressed_data_size = uncompressed_data_size;
        record_compressed->data_size = compressed_size + sizeof(uint32_t);
        
        buffer = &esp_buffer;
        buffer->now += record_compressed->data_size;
        compression_buffer.now = compression_buffer.start;

        inside_compressed_record = false;
    } else {
        record->data_size = (uint32_t)(buffer->now - (uint8_t*)record - sizeof(*record));
    }

    return record;
}

static uint8_t parse_hex_char(char c) {
    static const char alphabet[17] = "0123456789abcdef";
    for (int i = 0; _countof(alphabet); ++i) {
        if (c == alphabet[i]) {
            return i;
        }
    }
    verify(false);
    return 0;
}

FormID TextRecordReader::read_formid() {
    int nread = 0;
    FormID formid;
    int count = sscanf_s(now, "[%08X]%n", &formid.value, &nread);
    verify(count == 1);
    verify(nread == 1 + 8 + 1); // [DEADBEEF]
    now += 1 + 8 + 1;
    return formid;
}

void TextRecordReader::read_formid_line(Slice* slice) {
    auto line_end = peek_end_of_current_line();
    auto formid = read_formid();
    slice->write_struct(&formid);
    verify(now == line_end);
    now = line_end + 1; // +1 for '\n'.
}

void TextRecordReader::read_byte_array(Slice* slice, size_t count) {
    verify(slice->remaining_size() >= count);

    for (int i = 0; i < count; ++i) {
        uint8_t a = parse_hex_char(now[(i * 2) + 0]);
        uint8_t b = parse_hex_char(now[(i * 2) + 1]);
        slice->now[i] = (a << 4) | b;
    }
        
    slice->advance(count);
}

static bool has_custom_indent_rules(TypeKind kind) {
    switch (kind) {
        case TypeKind::Struct:
        case TypeKind::VMAD:
        case TypeKind::Vector3:
        case TypeKind::Filter:
            return true;
    }
    return false;
}

PapyrusPropertyType TextRecordReader::read_papyrus_object(Slice* slice, const VMAD_Header* header, PapyrusPropertyType expect_type) {
    verify(header->object_format == 2);

    auto type = PapyrusPropertyType::None;
    if (expect_type == PapyrusPropertyType::None) {
        for (size_t i = 0; i < Type_PapyrusPropertyType.field_count; ++i) {
            const auto& field = Type_PapyrusPropertyType.fields[i];
            if (try_begin_custom_struct(field.name)) {
                type = (PapyrusPropertyType)field.value;
                break;
            }
        }
    } else {
        const auto field = Type_PapyrusPropertyType.get_field_by_value((uint32_t)expect_type);
        verify(field);
        verify(try_begin_custom_struct(field->name));
        type = expect_type;
    }

    verify(type != PapyrusPropertyType::None);
    defer(end_custom_struct());
    
    switch (type) {
        case PapyrusPropertyType::Object: {
            const auto property = slice->advance<VMAD_PropertyObjectV2>();
            property->form_id = read_custom_field<FormID>("Form ID");
            property->alias = read_custom_field<uint16_t>("Alias");
        } break;

        case PapyrusPropertyType::String: {
            --indent;
            read_type<WString>(slice);
            ++indent;
        } break;

        case PapyrusPropertyType::Int: {
            --indent;
            read_type<int>(slice);
            ++indent;
        } break;

        case PapyrusPropertyType::Float: {
            --indent;
            read_type<float>(slice);
            ++indent;
        } break;

        case PapyrusPropertyType::Bool: {
            --indent;
            read_type<bool>(slice);
            ++indent;
        } break;

        case PapyrusPropertyType::ObjectArray:
        case PapyrusPropertyType::StringArray:
        case PapyrusPropertyType::IntArray:
        case PapyrusPropertyType::FloatArray:
        case PapyrusPropertyType::BoolArray: {
            verify(expect_type == PapyrusPropertyType::None);
            const auto inner_type = (PapyrusPropertyType)((uint32_t)type - 10);
            auto count = slice->advance<uint32_t>();
            while (try_continue_current_indent()) {
                *count += 1;
                read_papyrus_object(slice, header, inner_type);
            }
        } break;

        default: {
            verify(false);
        } break;
    }

    return type;
}

uint16_t TextRecordReader::read_papyrus_scripts(Slice* slice, const VMAD_Header* header) {
    uint16_t script_count = 0;

    while (try_begin_custom_struct("Script")) {
        defer(end_custom_struct());
        ++script_count;

        passthrough_custom_field<WString>(slice, "Name");

        if (header->version >= 4) {
            passthrough_custom_field<uint8_t>(slice, "Status");
        }

        const auto property_count = slice->advance<uint16_t>();
        while (try_begin_custom_struct("Property")) {
            defer(end_custom_struct());
            *property_count += 1;

            passthrough_custom_field<WString>(slice, "Name");
            auto type = slice->advance<PapyrusPropertyType>();
            if (header->version >= 4) {
                passthrough_custom_field<uint8_t>(slice, "Status");
            }

            *type = read_papyrus_object(slice, header);
        }
    }

    return script_count;
}

PapyrusFragmentFlags TextRecordReader::read_papyrus_info_record_fragment(Slice* slice, const char* fragment, PapyrusFragmentFlags flags) {
    if (!try_begin_custom_struct(fragment)) {
        return PapyrusFragmentFlags::None;
    }

    slice->write_constant<uint8_t>(1);
    passthrough_custom_field<WString>(slice, "Script Name");
    passthrough_custom_field<WString>(slice, "Fragment Name");
        
    end_custom_struct();
    return flags;
}

void TextRecordReader::read_string(Slice* slice) {
    if (expect("\"\"\"")) {
        verify(expect("\n"));
        auto had_at_least_one_newline = false;

        while (true) {
            expect_indent();
            if (expect("\"\"\"")) {
                verify(peek_end_of_current_line() == now); // ensure there is no trailing stuff
                if (had_at_least_one_newline) {
                    // Rollback last newline. This is ugly, but better than alternatives.
                    slice->now -= 2; 
                }
                ++now; // skip \n
                return;
            }

            while (now < end) {
                char c = *now++;
                if (c == '"') {
                    verify(false); // should be prefixed with backslash
                } else if (c == '\r') {
                    verify(false); // CRLF line endings are not supported
                } else if (c == '\n') {
                    slice->write_literal("\r\n");
                    had_at_least_one_newline = true;
                    goto next_line;
                } else if (c == '\\') {
                    verify(now < end && *now == '"');
                    slice->write_literal("\"");
                    ++now;
                } else {
                    slice->write_bytes(&c, 1);
                }
            }

            verify(false); // reached end of file
            next_line: {}
        }
    }
    
    if (!expect("\"")) {
        verify(false);
    }
 
    const auto line_end = peek_end_of_current_line();
    const auto count = (line_end - now) - 1;
    slice->write_bytes(now, count);
    verify(now[count] == '"');
    now = line_end + 1; // +1 for '\n'.
}

size_t TextRecordReader::read_type(Slice* slice, const Type* type) {
    ++indent;

    if (!has_custom_indent_rules(type->kind)) {
        expect_indent();
    }
        
    auto slice_now_before_parsing = slice->now;
        
    // @TODO: Cleanup: there are too many statements like "now = line_end + 1".
    switch (type->kind) {
        case TypeKind::ByteArray: {
            const auto line_end = peek_end_of_current_line();
            const auto count = line_end - now;
            verify((count % 2) == 0);

            read_byte_array(slice, count / 2);
                
            now = line_end + 1; // +1 for '\n'.
        } break;

        case TypeKind::ByteArrayCompressed: {
            const auto line_end = peek_end_of_current_line();
            const auto count = line_end - now;

            auto base64_buffer = compression_buffer.now;
            auto base64_size = (uLong)base64_decode(now, line_end - now, base64_buffer, compression_buffer.remaining_size());
            compression_buffer.now += base64_size;
               
            auto result_size = (uLongf)compression_buffer.remaining_size();
            const auto uncompressed_buffer = compression_buffer.now;
            const auto result = ::uncompress(uncompressed_buffer, &result_size, base64_buffer, base64_size);
            verify(result == Z_OK);

            compression_buffer.now = base64_buffer; // This line must be before slice->write_bytes, because slice can point to &compression_buffer
            slice->write_bytes(uncompressed_buffer, result_size);
                
            now = line_end + 1; // +1 for '\n'.
        } break;

        case TypeKind::ByteArrayFixed: {
            const auto line_end = peek_end_of_current_line();
            const auto count = (line_end - now) / 2;
            verify(((line_end - now) % 2) == 0);
            verify(count == type->size);

            read_byte_array(slice, count);

            now = line_end + 1; // +1 for '\n'.
        } break;

        case TypeKind::ByteArrayRLE: {
            const auto line_end = peek_end_of_current_line();
            const auto count = (line_end - now) / 2;
            verify(((line_end - now) % 2) == 0);

            const auto buffer = compression_buffer.now;
            auto buffer_now = buffer;

            for (int i = 0; i < count; ++i) {
                char c0 = now[(i * 2) + 0];
                char c1 = now[(i * 2) + 1];
                if (c0 == ByteArrayRLE_SequenceMarker_00 || c0 == ByteArrayRLE_SequenceMarker_FF) {
                    size_t repeats = 1ULL + ((size_t)c1 - (size_t)ByteArrayRLE_StreamStart);
                    verify(repeats <= ByteArrayRLE_MaxStreamValue);
                    memset(buffer_now, c0 == ByteArrayRLE_SequenceMarker_00 ? 0x00 : 0xFF, repeats);
                    buffer_now += repeats;
                    continue;
                }

                uint8_t a = parse_hex_char(c0);
                uint8_t b = parse_hex_char(c1);
                *buffer_now++ = (a << 4) | b;
            }

            slice->write_bytes(buffer, buffer_now - buffer);

            now = line_end + 1; // +1 for '\n'.
        } break;

        case TypeKind::ZString:
        case TypeKind::LString: {
            // @TODO: localized LString
            const auto sss = slice->now;
            read_string(slice);
            slice->write_literal("\0");
        } break;

        case TypeKind::WString: {
            auto count_small = slice->advance<uint16_t>();
            const auto string_start = slice->now;
            read_string(slice);
            const auto count = slice->now - string_start;
            verify(count >= 0 && count <= 0xffff);
            *count_small = static_cast<uint16_t>(count);
        } break;

        case TypeKind::Float: {
            auto line_end = peek_end_of_current_line();
            int nread = 0;
                
            switch (type->size) {
                case sizeof(float): {
                    float value;
                    const auto result = std::from_chars(now, line_end, value);
                    verify(result.ec == std::errc{});
                    verify(result.ptr == line_end);
                    slice->write_struct(&value);
                } break;

                case sizeof(double): {
                    double value;
                    const auto result = std::from_chars(now, line_end, value);
                    verify(result.ec == std::errc{});
                    verify(result.ptr == line_end);
                    slice->write_struct(&value);
                } break;
            }
                
            now = line_end + 1; // +1 for '\n'.
        } break;

        case TypeKind::Integer: {
            const auto integer_type = (const TypeInteger*)type;
            auto line_end = peek_end_of_current_line();
            int nread = 0;
            uint64_t value = 0;

            if (integer_type->is_unsigned) {
                int count = sscanf_s(now, "%llu%n", &value, &nread);
                verify(count == 1);
                verify(nread == line_end - now);
            } else {
                int count = sscanf_s(now, "%lld%n", &value, &nread);
                verify(count == 1);
                verify(nread == line_end - now);
            }

            slice->write_integer_of_size(value, integer_type->size);

            now = line_end + 1; // +1 for '\n'.
        } break;

        case TypeKind::Struct: {
            const auto struct_type = (const TypeStruct*)type;
            for (int i = 0; i < struct_type->field_count; ++i) {
                const auto& field = struct_type->fields[i];
                if (field.type->kind == TypeKind::Constant) {
                    const auto constant_type = (const TypeConstant*)field.type;
                    slice->write_bytes(constant_type->bytes, constant_type->size);
                    continue;
                }
                expect_indent();
                verify(expect(field.name));
                verify(expect("\n"));
                read_type(slice, field.type);
            }
        } break;

        case TypeKind::FormID: {
            read_formid_line(slice);
        } break;

        case TypeKind::FormIDArray: {
            read_formid_line(slice);
            while (try_continue_current_indent()) {
                expect_indent();
                read_formid_line(slice);
            }
        } break;

        case TypeKind::Enum: {
            const auto enum_type = (const TypeEnum*)type;

            uint32_t result = 0;

            if (enum_type->flags) {
                while (true) {
                    if (!expect("+ ")) {
                        verify(now + 1 < end && now[0] == '\n');
                        ++now;
                        break;
                    }

                    auto line_end = peek_end_of_current_line();
                    auto count = line_end - now;
                    verify(count > 0);

                    for (int i = 0; i < enum_type->field_count; ++i) {
                        const auto& flag = enum_type->fields[i];
                        if (strlen(flag.name) == count && memory_equals(flag.name, now, count)) {
                            result |= flag.value;
                            goto parse_flag_ok;
                        }
                    }

                    uint32_t unrecognized_flags;
                    int nread;
                    verify(1 == _snscanf_s(now, count, "%X%n", &unrecognized_flags, &nread));
                    verify(nread == count);

                    result |= unrecognized_flags;

                    parse_flag_ok:
                    now = line_end + 1;

                    if (!try_continue_current_indent()) {
                        break;
                    }
                    expect_indent();
                }
            } else {
                auto line_end = peek_end_of_current_line();
                auto count = line_end - now;
                
                for (int i = 0; i < enum_type->field_count; ++i) {
                    const auto& field = enum_type->fields[i];
                    if (strlen(field.name) == count && 0 == memcmp(now, field.name, count)) {
                        result = field.value;
                        now = line_end + 1; // +1 for '\n'.
                        goto parse_ok;
                    }
                }
                
                verify(false); // Unknown field.
                parse_ok: {};
            }

            slice->write_integer_of_size(result, enum_type->size);
        } break;

        case TypeKind::Boolean: {
            const auto line_end = peek_end_of_current_line();
            if (expect("True\n")) {
                const auto value = true;
                slice->write_struct(&value);
            } else if (expect("False\n")) {
                const auto value = false;
                slice->write_struct(&value);
            } else {
                verify(false);
            }
        } break;

        case TypeKind::VMAD: {
            auto header = slice->advance<VMAD_Header>();
            header->version = read_custom_field<uint16_t>("Version");
            header->object_format = read_custom_field<uint16_t>("Object Format");
            verify(header->version >= 2 && header->version <= 5);
            verify(header->object_format >= 1 && header->object_format <= 2);

            header->script_count = read_papyrus_scripts(slice, header);

            if (!try_continue_current_indent()) {
                break;
            }

            switch (current_record_type) {
                case RecordType::INFO: {
                    slice->write_constant<uint8_t>(2); // version?

                    const auto flags = slice->advance<PapyrusFragmentFlags>();
                    passthrough_custom_field<WString>(slice, "Fragment Script File Name");

                    *flags |= read_papyrus_info_record_fragment(slice, "Start Fragment", PapyrusFragmentFlags::HasBeginScript);
                    *flags |= read_papyrus_info_record_fragment(slice, "End Fragment", PapyrusFragmentFlags::HasEndScript);
                } break;

                case RecordType::QUST: {
                    slice->write_constant<uint8_t>(2); // version?

                    const auto fragment_count = slice->advance<uint16_t>();
                    passthrough_custom_field<WString>(slice, "File Name");

                    while (try_begin_custom_struct("Fragment")) {
                        defer(end_custom_struct());
                        *fragment_count += 1;

                        passthrough_custom_field<uint16_t>(slice, "Index");
                        slice->write_constant<uint16_t>(0);
                        passthrough_custom_field<uint32_t>(slice, "Log Entry");
                        slice->write_constant<uint8_t>(1);
                        passthrough_custom_field<WString>(slice, "Script Name");
                        passthrough_custom_field<WString>(slice, "Function Name");
                    }

                    const auto alias_count = slice->advance<uint16_t>();
                    while (try_begin_custom_struct("Alias")) {
                        defer(end_custom_struct());
                        *alias_count += 1;

                        read_papyrus_object(slice, header, PapyrusPropertyType::Object);
                        slice->write_constant<uint16_t>(header->version);
                        slice->write_constant<uint16_t>(header->object_format);

                        const auto script_count = slice->advance<uint16_t>();
                        *script_count = read_papyrus_scripts(slice, header);
                    }
                } break;
            }
        } break;

        case TypeKind::Filter: {
            const auto filter_type = (const TypeFilter*)type;
            --indent;
            read_type(slice, filter_type->inner_type);
            ++indent;
        } break;

        case TypeKind::Vector3: {
            --indent;
            read_type(slice, &Type_float);
            read_type(slice, &Type_float);
            read_type(slice, &Type_float);
            ++indent;
        } break;

        default: {
            verify(false);
        } break;
    }

    // @TODO: fix everything else before doing this
    //if (!has_custom_indent_rules(type->kind)) {
    //    expect("\n");
    //}

    --indent;
    return slice->now - slice_now_before_parsing;
}

bool TextRecordReader::try_begin_custom_struct(const char* header_name) {
    if (try_continue_current_indent()) {
        const auto start = &now[indent * 2];
        const auto line_end = peek_end_of_current_line();
        const auto count = line_end - start;
        if (count == strlen(header_name) && memory_equals(start, header_name, count)) {
            now = line_end + 1; // skip \n
            ++indent;
            return true;
        }
    }
    return false;
}

void TextRecordReader::end_custom_struct() {
    --indent;
}

void TextRecordReader::read_custom_field(Slice* slice, const char* field_name, const Type* type) {
    expect_indent();
    verify(expect(field_name));
    verify(expect("\n"));

    read_type(slice, type);
}

void TextRecordReader::read_field(const RecordFieldDef* field_def) {
    expect_indent();
    
    const auto field = buffer->advance<RawRecordField>();
    field->type = read_record_field_type();
    
    skip_to_next_line();

    read_type(buffer, field_def ? field_def->data_type : &Type_ByteArray);

    const auto size = buffer->now - reinterpret_cast<const uint8_t*>(field) - sizeof(*field);
    verify(size <= 0xffff);
    field->size = static_cast<uint16_t>(size);
}

void TextRecordReader::read_subrecord_fields(const RecordFieldDefSubrecord* field_def) {
    for (const auto inner_field_def : field_def->fields) {
        if (inner_field_def->data_type->kind == TypeKind::Constant) {
            const auto constant_type = (const TypeConstant*)inner_field_def->data_type;

            buffer->write_constant<RawRecordField>({ inner_field_def->type, static_cast<uint16_t>(constant_type->size) });
            buffer->write_bytes(constant_type->bytes, constant_type->size);

            continue;
        }

        read_field(inner_field_def);
    }
}

RecordType TextRecordReader::read_record_type() {
    verify(now + 4 <= end);
    auto result = *(RecordType*)now;
    now += 4;
    return result;
}

RecordFieldType TextRecordReader::read_record_field_type() {
    verify(now + 4 < end);
    auto result = *(RecordFieldType*)now;
    now += 4;
    return result;
}

bool TextRecordReader::expect(const char* string) {
    auto count = strlen(string);
    if (now + count > end) {
        return false;
    }

    if (memory_equals(now, string, count)) {
        now += count;
        return true;
    }
        
    return false;
}

int TextRecordReader::expect_int() {
    int value = 0;
    int nread = 0;
    verify(1 == _snscanf_s(now, end - now, "%d%n", &value, &nread));
    now += nread;
    return value;
}

bool TextRecordReader::try_continue_current_indent() {
    const auto current_indent = peek_indents();
    if (current_indent == indent) {
        return true;
    }
    verify(current_indent < indent);
    return false;
}

int TextRecordReader::peek_indents() {
    int indents = 0;
    auto curr = now;
    while (curr + 2 < end) {
        if (memory_equals(curr, "  ", 2)) {
            ++indents;
            curr += 2;
        } else {
            break;
        }
    }
    return indents;
}

const char* TextRecordReader::peek_end_of_current_line() {
    auto curr = now;
    while (curr < end) {
        char c = *curr;
        if (c == '\n') {
            return curr;
        } else {
            ++curr;
        }
    }
    return end;
}

void TextRecordReader::expect_indent() {
    for (int i = 0; i < indent; ++i) {
        verify(expect("  "));
    }
    verify(now >= end || now[0] != ' ');
}

void TextRecordReader::skip_to_next_line() {
    while (now < end) {
        char c = *now;
        if (c == '\n') {
            ++now;
            break;
        } else {
            ++now;
        }
    }
}

void text_to_esp(const wchar_t* text_path, const wchar_t* esp_path) {
    TextRecordReader reader;
    reader.init();
    defer(reader.dispose());

    const auto text = read_file(text_path);
    reader.read_records((const char*)text.data, (const char*)text.data + text.count);

    write_file(esp_path, { reader.buffer->start, reader.buffer->size() });
}
