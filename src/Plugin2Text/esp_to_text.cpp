#include "esp_to_text.hpp"
#include "os.hpp"
#include "base64.hpp"
#include <stdio.h>
#include <zlib.h>
#include <charconv>

void TextRecordWriter::init(ProgramOptions options) {
    output_buffer = allocate_virtual_memory(1024 * 1024 * 64);
    scratch_buffer = allocate_virtual_memory(1024 * 1024 * 32);
    
    this->options = options;
}

void TextRecordWriter::dispose() {
    free_virtual_memory(&output_buffer);
    free_virtual_memory(&scratch_buffer);
}

void TextRecordWriter::write_format(_Printf_format_string_ const char* format, ...) {
    va_list args;
    va_start(args, format);
    int count = vsnprintf((char*)output_buffer.now, output_buffer.remaining_size(), format, args);
    va_end(args);
    verify(count > 0);
    output_buffer.now += count;
}

void TextRecordWriter::write_byte_array(const uint8_t* data, size_t size) {
    auto buffer = output_buffer.advance(size * 2);
    static const char alphabet[17] = "0123456789abcdef";

    for (size_t i = 0; i < size; ++i) {
        uint8_t c = data[i];
        buffer[(i * 2) + 0] = alphabet[c / 16];
        buffer[(i * 2) + 1] = alphabet[c % 16];
    }
}

void TextRecordWriter::write_indent() {
    const auto bytes = indent * 2;
    auto buffer = output_buffer.advance(bytes);
    memset(buffer, ' ', bytes);
}

void TextRecordWriter::write_newline() {
    write_literal("\n");
}

void TextRecordWriter::write_string(const char* str) {
    write_bytes(str, strlen(str));
}

void TextRecordWriter::write_bytes(const void* data, size_t size) {
    verify(output_buffer.now + size < output_buffer.end);
    memcpy(output_buffer.now, data, size);
    output_buffer.now += size;
}

void TextRecordWriter::write_records(const Array<RecordBase*> records) {
    write_literal("plugin2text version 1.00\n---\n");

    for (const auto record : records) {
        write_record(record);
    }
}

void TextRecordWriter::write_grup_record(const GrupRecord* record) {
    if (record->group_type != RecordGroupType::Top) {
        write_format(" - %s", record_group_type_to_string(record->group_type));
            
        switch (record->group_type) {
            case RecordGroupType::ExteriorCellBlock:
            case RecordGroupType::ExteriorCellSubBlock: {
                write_format(" (%d; %d)", record->grid_x, record->grid_y);
            } break;

            case RecordGroupType::InteriorCellBlock:
            case RecordGroupType::InteriorCellSubBlock: {
                write_format(" %d", (int)record->label);
            } break;

            default: {
                write_format(" [%08X]", record->label);
            } break;
        }
    }

    write_record_timestamp(record->timestamp);
    write_record_unknown(record->unknown);
    write_newline();

    ++indent;
    for (const auto record : record->records) {
        write_record(record);
    }
    --indent;
}

RecordFlags TextRecordWriter::write_flags(RecordFlags flags, const RecordDef* def) {
    for (size_t i = 0; i < def->flags.count; ++i) {
        const auto& flag = def->flags.data[i];
        if (is_bit_set(flags, flag.bit)) {
            write_newline();
            write_indent();
            write_literal("+ ");
            write_string(flag.name);
            flags = clear_bit(flags, flag.bit);
        }
    }
    return flags;
}

void TextRecordWriter::write_record_timestamp(uint16_t timestamp) {
    if (timestamp && is_bit_set(options, ProgramOptions::ExportTimestamp)) {
        write_newline();
        ++indent;
        write_indent();
        --indent;
        int y = (timestamp & 0b1111111'0000'00000) >> 9;
        int m = (timestamp & 0b0000000'1111'00000) >> 5;
        int d = (timestamp & 0b0000000'0000'11111);
        verify(m >= 1 && m <= 12);
        write_format("%d %s 20%d", d, month_to_short_string(m), y);
    }
}

void TextRecordWriter::write_record_unknown(uint32_t unknown) {
    if (unknown) {
        write_newline();
        ++indent;
        write_indent();
        --indent;
        write_format("Unknown = %X", unknown);
    }
}

void TextRecordWriter::write_record(const RecordBase* record_base) {
    current_record_type = record_base->type;
    write_indent();
    write_bytes(&record_base->type, 4);

    if (record_base->type == RecordType::GRUP) {
        write_grup_record((const GrupRecord*)record_base);
        return;
    }

    auto record = (const Record*)record_base;
    write_format(" [%08X]", record->id.value);
    if (record->version != 44) {
        write_format(",v%d", record->version);
    }

    auto def = get_record_def(record->type);
    if (def && def->comment) {
        write_literal(" - ");
        write_string(def->comment);
    }

    write_record_timestamp(record->timestamp);
    write_record_unknown(record->unknown);

    if (record->flags != RecordFlags::None) {
        auto flags = record->flags;
        ++indent;
        if (def) {
            flags = write_flags(flags, def);
        }
        flags = write_flags(flags, &Record_Common);
        if (flags != RecordFlags::None) {
            write_newline();
            write_indent();
            write_format("+ %X", flags);
        }
        --indent;
    }

    if (!def) {
        def = &Record_Common;
    }

    write_newline();

    for (int i = 0; i < record->fields.count;) {
        const auto field = record->fields[i];

        auto field_def = def->get_field_def(field->type);
        if (!field_def) {
            field_def = Record_Common.get_field_def(field->type);
        }

        if (!field_def || field_def->def_type == RecordFieldDefType::Field) {
            write_field(field, static_cast<const RecordFieldDef*>(field_def));
            ++i;
        } else if (field_def->def_type == RecordFieldDefType::Subrecord) {
            const auto subrecord_field_def = static_cast<const RecordFieldDefSubrecord*>(field_def);
            write_subrecord_fields(subrecord_field_def, { &record->fields.data[i], static_cast<size_t>(record->fields.count - i) });
            i += static_cast<int>(subrecord_field_def->fields.count);
        } else {
            verify(false);
        }
    }
}

static bool has_custom_indent_rules(TypeKind kind) {
    switch (kind) {
        case TypeKind::Struct:
        case TypeKind::Constant:
        case TypeKind::Filter:
        case TypeKind::Vector3:
        case TypeKind::VMAD:
            return true;
    }
    return false;
}

static size_t count_bytes(const uint8_t* start, const uint8_t* end, uint8_t byte) {
    auto now = start;
    while (now < end) {
        if (*now != byte) {
            return now - start;
        }
        ++now;
    }
    return end - start;
}

void TextRecordWriter::write_papyrus_object(const VMAD_Field& vmad, const VMAD_ScriptPropertyValue& value, PapyrusPropertyType type) {
    verify(vmad.object_format == 2);

    const auto type_field = Type_PapyrusPropertyType.get_field_by_value((uint32_t)type);
    verify(type_field);
    const auto type_name = type_field->name;
    
    switch (type) {
        case PapyrusPropertyType::Object: {
            begin_custom_struct(type_name);
            defer(end_custom_struct());

            write_custom_field("Form ID", value.as_object.form_id);
            write_custom_field("Alias", value.as_object.alias);
        } break;

        case PapyrusPropertyType::String: {
            write_custom_field(type_name, value.as_string);
        } break;

        case PapyrusPropertyType::Int: {
            write_custom_field(type_name, value.as_int);
        } break;

        case PapyrusPropertyType::Float: {
            write_custom_field(type_name, value.as_float);
        } break;

        case PapyrusPropertyType::Bool: {
            write_custom_field(type_name, value.as_bool);
        } break;

        case PapyrusPropertyType::ObjectArray:
        case PapyrusPropertyType::StringArray:
        case PapyrusPropertyType::IntArray:
        case PapyrusPropertyType::FloatArray:
        case PapyrusPropertyType::BoolArray: {
            begin_custom_struct(type_name);
            defer(end_custom_struct());

            const auto inner_type = (PapyrusPropertyType)((uint32_t)type - 10);
            for (uint32_t i = 0; i < value.as_array.count; ++i) {
                write_papyrus_object(vmad, value.as_array.values[i], inner_type);
            }
        } break;

        default: {
            verify(false);
        } break;
    }
}

void TextRecordWriter::write_papyrus_scripts(const VMAD_Field& vmad, const Array<VMAD_Script>& scripts) {
    for (const auto& script : scripts) {
        begin_custom_struct("Script");
        defer(end_custom_struct());

        write_custom_field("Name", script.name);

        if (vmad.version >= 4) {
            write_custom_field("Status", script.status);
        }

        for (const auto& property : script.properties) {
            begin_custom_struct("Property");
            defer(end_custom_struct());

            write_custom_field("Name", property.name);

            if (vmad.version >= 4) {
                write_custom_field("Status", property.status);
            }

            write_papyrus_object(vmad, property.value, property.type);
        }
    }
}

void TextRecordWriter::write_papyrus_info_record_fragment(const VMAD_Field& vmad, const char* name, const VMAD_INFO_Fragment& fragment) {
    begin_custom_struct(name);

    write_custom_field("Script Name", fragment.script_name);
    write_custom_field("Fragment Name", fragment.fragment_name);

    end_custom_struct();
}

void TextRecordWriter::write_string(const char* text, size_t count) {
    auto now = text;
    const auto end = now + count;
    auto multiline = false;
    while (now < end) {
        const auto c = *now++;
        if (c == '\r' || c == '\n' || c == '"') {
            multiline = true;
            continue;
        }
        verify(c >= 32 && c < 127);
    }

    now = text;
    if (multiline) {
        write_literal("\"\"\"\n");
        write_indent();
        while (now < end) {
            const auto c = *now++;
            if (c == '\r') {
                verify(now < end && *now == '\n');
                ++now;
                write_newline();
                write_indent();
            } else if (c == '\"') {
                write_literal("\\\"");
            } else {
                write_bytes(&c, 1);
            }
        }
        write_newline();
        write_indent();
        write_literal("\"\"\"");
    } else {
        write_literal("\"");
        write_bytes(text, count);
        write_literal("\"");
    }
}

void TextRecordWriter::write_type(const Type* type, const void* value, size_t size) {
    ++indent;

    if (!has_custom_indent_rules(type->kind)) {
        write_indent();
    }

    switch (type->kind) {
        case TypeKind::ZString: {
            if (size == 0) {
                write_literal("\"\"");
            } else {
                write_string((const char*)value, size - 1);
            }
        } break;

        case TypeKind::LString: {
            verify(!localized_strings);
            if (size == 0) {
                write_literal("\"\"");
            } else {
                write_string((const char*)value, size - 1);
            }
        } break;

        case TypeKind::WString: {
            auto wstr = (WString*)value;
            if (wstr->count == 0) {
                write_literal("\"\"");
            } else {
                write_string((const char*)wstr->data, wstr->count);
            }
        } break;

        case TypeKind::ByteArray: {
            write_byte_array((uint8_t*)value, size);
        } break;

        case TypeKind::ByteArrayCompressed: {
            auto buffer = scratch_buffer.advance(size);

            auto compressed_size = static_cast<uLongf>(scratch_buffer.remaining_size());
            auto result = ::compress(buffer, &compressed_size, (const uint8_t*)value, static_cast<uLong>(size));
            verify(result == Z_OK);

            scratch_buffer.now += compressed_size;

            auto output_size = base64_encode(buffer, compressed_size, (char*)scratch_buffer.now, scratch_buffer.end - scratch_buffer.now);
            write_bytes(scratch_buffer.now, output_size);
                
            write_byte_array(buffer, compressed_size);

            scratch_buffer.now = buffer;
        } break;

        case TypeKind::ByteArrayFixed: {
            verify(type->size == size);
            write_byte_array((uint8_t*)value, size);
        } break;

        case TypeKind::ByteArrayRLE: {
            auto buffer = output_buffer.advance(size * 2); // We actually may need less than "size * 2", but whatever.
            auto data = (uint8_t*)value;
            static const char alphabet[17] = "0123456789abcdef";
            size_t bytes_written = 0;

            for (size_t i = 0; i < size; ++i) {
                uint8_t c = data[i];
                if ((c == 0x00 || c == 0xFF) && i + 1 < size) {
                    size_t repeats = 1 + count_bytes(&data[i + 1], &data[size], c);
                    if (repeats > 1) {
                        if (repeats > ByteArrayRLE_MaxStreamValue) {
                            repeats = ByteArrayRLE_MaxStreamValue;
                        }
                         
                        buffer[bytes_written++] = c == 0x00 ? ByteArrayRLE_SequenceMarker_00 : ByteArrayRLE_SequenceMarker_FF;
                        buffer[bytes_written++] = ByteArrayRLE_StreamStart + ((char)repeats - 1);
                        i += repeats - 1;
                        continue;
                    }
                }

                buffer[bytes_written++] = alphabet[c / 16];
                buffer[bytes_written++] = alphabet[c % 16];
            }

            output_buffer.now = buffer + bytes_written;
        } break;

        case TypeKind::Integer: {
            verify(type->size == size);
            auto integer_type = (const TypeInteger*)type;
            if (integer_type->is_unsigned) {
                switch (size) {
                    case 1: write_format("%u", *(uint8_t*)value); break;
                    case 2: write_format("%u", *(uint16_t*)value); break;
                    case 4: write_format("%u", *(uint32_t*)value); break;
                    case 8: write_format("%llu", *(uint64_t*)value); break;
                }
            } else {
                switch (size) {
                    case 1: write_format("%d", *(int8_t*)value); break;
                    case 2: write_format("%d", *(int16_t*)value); break;
                    case 4: write_format("%d", *(int32_t*)value); break;
                    case 8: write_format("%lld", *(int64_t*)value); break;
                }
            }
        } break;

        case TypeKind::Float: {
            verify(type->size == size);
            switch (size) {
                case sizeof(float): {
                    const auto result = std::to_chars((char*)output_buffer.now, (char*)output_buffer.end, *(float*)value);
                    verify(result.ec == std::errc{});
                    output_buffer.now = (uint8_t*)result.ptr;
                } break;

                case sizeof(double): {
                    const auto result = std::to_chars((char*)output_buffer.now, (char*)output_buffer.end, *(double*)value);
                    verify(result.ec == std::errc{});
                    output_buffer.now = (uint8_t*)result.ptr;
                } break;
            }
        } break;

        case TypeKind::Struct: {
            verify(type->size == size);
            auto struct_type = (const TypeStruct*)type;
            size_t offset = 0;
            for (int i = 0; i < struct_type->field_count; ++i) {
                const auto& field = struct_type->fields[i];
                if (field.type->kind == TypeKind::Constant) {
                    write_type(field.type, (uint8_t*)value + offset, field.type->size);
                    offset += field.type->size;
                    continue;
                }

                write_indent();
                write_string(field.name);
                write_newline();

                write_type(field.type, (uint8_t*)value + offset, field.type->size);
                offset += field.type->size;
            }
            verify(offset == size);
        } break;

        case TypeKind::FormID: {
            verify(type->size == size);
            verify(size == sizeof(int));
            write_format("[%08X]", *(int*)value);
        } break;

        case TypeKind::FormIDArray: {
            verify((size % sizeof(int)) == 0);
            const size_t keyword_count = size / sizeof(int);
            for (size_t i = 0; i < keyword_count; ++i) {
                write_format("[%08X]", ((int*)value)[i]);
                if (i != keyword_count - 1) {
                    write_newline();
                    write_indent();
                }
            }
        } break;

        case TypeKind::Enum: {
            verify(type->size == size);
            const auto enum_type = (const TypeEnum*)type;
                
            uint32_t enum_value = 0;
            switch (size) {
                case 1: enum_value = *(uint8_t*)value; break;
                case 2: enum_value = *(uint16_t*)value; break;
                case 4: enum_value = *(uint32_t*)value; break;
                case 8: verify(false); break; //enum_value = *(uint64_t*)value; break;
                default: verify(false); break;
            }

            if (enum_type->flags) {
                verify(type->size <= sizeof(uint32_t));

                for (size_t i = 0; i < enum_type->field_count; ++i) {
                    const auto& field = enum_type->fields[i];
                    if (enum_value & field.value) {
                        write_literal("+ ");
                        write_string(field.name);

                        enum_value = clear_bit(enum_value, field.value);
                        if (enum_value == 0) {
                            break; // fast exit if wrote all bits.
                        } else {
                            write_newline();
                            write_indent();
                        }
                    }
                }

                if (enum_value) {
                    write_format("+ %X", enum_value);
                }
            } else {
                for (size_t i = 0; i < enum_type->field_count; ++i) {
                    const auto& field = enum_type->fields[i];
                    if (field.value == enum_value) {
                        write_string(field.name);
                        goto ok;
                    }
                }

                write_format("%u", enum_value);
                ok: break;
            }
        } break;

        case TypeKind::Boolean: {
            verify(type->size == sizeof(bool));
            if (*(bool*)value) {
                write_literal("True");
            } else {
                write_literal("False");
            }
        } break;

        case TypeKind::VMAD: {
            VMAD_Field vmad;
            vmad.parse(static_cast<const uint8_t*>(value), size, current_record_type);

            write_custom_field("Version", vmad.version);
            write_custom_field("Object Format", vmad.object_format);

            write_papyrus_scripts(vmad, vmad.scripts);

            // @NOTE: instead of using "current_record" we can make VMAD_INFO, VMAD_QUST, etc...
            if (vmad.contains_record_specific_info) {
                switch (current_record_type) {
                    case RecordType::INFO: {
                        write_custom_field("Fragment Script File Name", vmad.info.script_name);

                        if (is_bit_set(vmad.info.flags, PapyrusFragmentFlags::HasBeginScript)) {
                            write_papyrus_info_record_fragment(vmad, "Start Fragment", vmad.info.start_fragment);
                        }
                        if (is_bit_set(vmad.info.flags, PapyrusFragmentFlags::HasEndScript)) {
                            write_papyrus_info_record_fragment(vmad, "End Fragment", vmad.info.end_fragment);
                        }
                    } break;

                    case RecordType::QUST: {
                        write_custom_field("File Name", vmad.qust.file_name);

                        for (const auto& fragment : vmad.qust.fragments) {
                            begin_custom_struct("Fragment");
                            defer(end_custom_struct());

                            write_custom_field("Index", fragment.index);
                            write_custom_field("Log Entry", fragment.log_entry);
                            write_custom_field("Script Name", fragment.script_name);
                            write_custom_field("Function Name", fragment.function_name);
                        }

                        for (const auto& alias : vmad.qust.aliases) {
                            begin_custom_struct("Alias");
                            defer(end_custom_struct());

                            VMAD_ScriptPropertyValue value;
                            value.as_object = alias.object;

                            write_papyrus_object(vmad, value, PapyrusPropertyType::Object);
                            write_papyrus_scripts(vmad, alias.scripts);
                        }
                    } break;
                }
            }
        } break;

        case TypeKind::Constant: {
            verify(type->size == size);
            const auto constant_type = (const TypeConstant*)type;
            verify(memory_equals(value, constant_type->bytes, size));
        } break;

        case TypeKind::Filter: {
            const auto filter_type = (const TypeFilter*)type;

            --indent;
            if (is_bit_set(options, ProgramOptions::PreserveJunk)) {
                write_type(filter_type->inner_type, value, size);
            } else {
                const auto highwater = scratch_buffer.now;
                const auto preprocessed_value = scratch_buffer.advance(size);
                memcpy(preprocessed_value, value, size);

                filter_type->preprocess(preprocessed_value, size);
                write_type(filter_type->inner_type, preprocessed_value, size);

                scratch_buffer.now = highwater;
            }
            ++indent;
        } break;

        case TypeKind::Vector3: {
            // @TODO: maybe can replace with fixed array type.
            verify(size == sizeof(Vector3));
            const auto vector = (const Vector3*)value;
            --indent;
            write_type(&Type_float, &vector->x, sizeof(vector->x));
            write_type(&Type_float, &vector->y, sizeof(vector->y));
            write_type(&Type_float, &vector->z, sizeof(vector->z));
            ++indent;
        } break;

        default: {
            verify(false);
        } break;
    }
   
    if (!has_custom_indent_rules(type->kind)) {
        write_newline();
    }

    --indent;
}

void TextRecordWriter::write_field(const RecordField* field, const RecordFieldDef* field_def) {
    ++indent;
    write_indent();

    write_bytes(&field->type, 4);

    if (field_def && field_def->comment) {
        write_literal(" - ");
        write_string(field_def->comment);
    }

    write_newline();

    const auto data_type = field_def ? field_def->data_type : &Type_ByteArray;
    write_type(data_type, field->data.data, field->data.count);
        
    --indent;
}

void TextRecordWriter::write_subrecord_fields(const RecordFieldDefSubrecord* field_def, StaticArray<RecordField*> fields) {
    for (int field_def_index = 0, processed_field_index = 0; field_def_index < field_def->fields.count; ++field_def_index) {
        const auto inner_field_def = (const RecordFieldDef*)field_def->fields.data[field_def_index];
        verify(inner_field_def->def_type == RecordFieldDefType::Field);

        if (inner_field_def->data_type->kind == TypeKind::Constant) {
            continue;
        }

        verify(processed_field_index < fields.count);
        write_field(fields.data[processed_field_index++], inner_field_def);
    }
}

void TextRecordWriter::begin_custom_struct(const char* header_name) {
    write_indent();
    write_string(header_name);
    write_newline();
    ++indent;
}

void TextRecordWriter::end_custom_struct() {
    --indent;
}

void TextRecordWriter::write_custom_field(const char* field_name, const Type* type, const void* value, size_t size) {
    write_indent();
    write_string(field_name);
    write_newline();
    write_type(type, value, size);
}

void esp_to_text(ProgramOptions options, const EspObjectModel& model, const wchar_t* text_path) {
    TextRecordWriter writer;
    writer.init(options);
    defer(writer.dispose());

    writer.write_records(model.records);
    write_file(text_path, { writer.output_buffer.start, writer.output_buffer.size() });
}
