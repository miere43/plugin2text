#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "esp_to_text.hpp"
#include "os.hpp"
#include "tes.hpp"
#include "common.hpp"
#include "typeinfo.hpp"
#include <stdio.h>
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.h"
#include "base64.hpp"

struct TextRecordWriter {
    VirtualMemoryBuffer scratch_buffer;

    HANDLE output_handle = 0;
    int indent = 0;
    bool localized_strings = false; // @TODO: load value from TES4 record

    Record* current_record = nullptr; // Sometimes ESP deserialization depends on record type.

    void open(const wchar_t* path) {
        verify(!output_handle);
        output_handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        verify(output_handle != INVALID_HANDLE_VALUE);

        scratch_buffer = VirtualMemoryBuffer::alloc(1024 * 1024 * 32);

        write_format("plugin2text version 1.00\n---\n");
    }

    void close() {
        verify(output_handle);
        CloseHandle(output_handle);
        output_handle = 0;
    }

    void write_format(const char* format, ...) {
        char buffer[4096];

        va_list args;
        va_start(args, format);
        int count = vsprintf_s(buffer, format, args);
        va_end(args);

        verify(count > 0);
        write_bytes(buffer, count);
    }

    void write_byte_array(const uint8_t* data, size_t size) {
        auto buffer = new uint8_t[size * 2];
        static const char alphabet[17] = "0123456789abcdef";

        for (size_t i = 0; i < size; ++i) {
            uint8_t c = data[i];
            buffer[(i * 2) + 0] = alphabet[c / 16];
            buffer[(i * 2) + 1] = alphabet[c % 16];
        }

        write_bytes(buffer, size * 2);
        delete[] buffer;
    }

    void write_indent() {
        for (int i = 0; i < indent; ++i) {
            write_bytes("  ", 2);
        }
    }

    void write_newline() {
        write_bytes("\n", 1);
    }

    void write_string(const char* str) {
        write_bytes(str, strlen(str));
    }

    template<size_t N>
    void write_literal(const char(&data)[N]) {
        write_bytes(data, N - 1);
    }

    void write_bytes(const void* data, size_t size) {
        // @TODO: add buffering
        DWORD written = 0;
        verify(WriteFile(output_handle, data, (uint32_t)size, &written, nullptr));
        verify(written == size);
    }

    void write_records(const uint8_t* start, const uint8_t* end) {
        const uint8_t* now = start;
        while (now < end) {
            auto record = (Record*)now;
            write_record(record);
            write_newline();
            now += record->data_size + (record->type == RecordType::GRUP ? 0 : sizeof(Record));
        }
    }

    void write_grup_record(GrupRecord* record) {
        const uint8_t* now = (uint8_t*)record + sizeof(GrupRecord);
        const uint8_t* end = now + record->group_size - sizeof(GrupRecord);

        //write_format(" %04X", record->timestamp);
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

        indent += 1;
        while (now < end) {
            auto subrecord = (Record*)now;
            write_newline();
            write_indent();
            write_record(subrecord);
            now += subrecord->data_size + (subrecord->type == RecordType::GRUP ? 0 : sizeof(Record));
        }
        indent -= 1;
    }

    RecordFlags write_flags(RecordFlags flags, const RecordDef* def) {
        for (size_t i = 0; i < def->flags.count; ++i) {
            const auto& flag = def->flags.data[i];
            if ((uint32_t)flags & flag.bit) {
                write_newline();
                write_indent();
                write_literal("+ ");
                write_string(flag.name);
                flags = clear_bit(flags, (RecordFlags)flag.bit);
            }
        }
        return flags;
    }

    void write_record(Record* record) {
        current_record = record;
        write_bytes(&record->type, 4);
        auto def = get_record_def(record->type);

        if (record->type != RecordType::GRUP) {
            write_format(" [%08X]", record->id.value);
            if (record->version != 44) {
                write_format(",v%d", record->version);
            }
        }

        if (def && def->comment) {
            write_bytes(" - ", 3);
            write_bytes(def->comment, strlen(def->comment));
        }

        if (record->type == RecordType::GRUP) {
            write_grup_record((GrupRecord*)record);
            return;
        }

        // @TODO: write all flags
        if (record->flags != RecordFlags::None) {
            auto flags = record->flags;
            indent += 1;
            if (def) {
                flags = write_flags(flags, def);
            }
            flags = write_flags(flags, &Record_Common);
            if (flags != RecordFlags::None) {
                write_newline();
                write_indent();
                write_format("+ %X", flags);
            }
            indent -= 1;
        }

        const uint8_t* now;
        const uint8_t* end;
        if (record->is_compressed()) {
            uint32_t size = 0;
            now = record->uncompress(&size);
            end = now + size;
        } else {
            now = (uint8_t*)record + sizeof(Record);
            end = now + record->data_size;
        }

        if (!def) {
            def = &Record_Common;
        }

        indent += 1;
        while (now < end) {
            auto field = (RecordField*)now;
            write_newline();
            write_indent();
            indent += 1;
            write_field(def, field);
            indent -= 1;
            now += sizeof(RecordField) + field->size;
        }
        indent -= 1;
    }

    static void validate_ascii(const char* now, size_t count) {
        const auto end = now + count;
        while (now < end) {
            char c = *now++;
            verify(c >= 32 && c < 127); // @TODO: Escape string
        }
    }

    void write_type(const Type* type, const void* value, size_t size) {
        switch (type->kind) {
            case TypeKind::ZString: {
                if (size == 0) {
                    write_bytes("\"\"", 2);
                } else {
                    validate_ascii((char*)value, size - 1);
                    write_bytes("\"", 1);
                    write_bytes(value, size - 1);
                    write_bytes("\"", 1);
                }
            } break;

            case TypeKind::LString: {
                verify(!localized_strings);
                if (size == 0) {
                    write_bytes("\"\"", 2);
                } else {
                    write_bytes("\"", 1);
                    write_bytes(value, size - 1);
                    write_bytes("\"", 1);
                }
            } break;

            case TypeKind::WString: {
                auto wstr = (WString*)value;
                if (wstr->count == 0) {
                    write_literal("\"\"");
                } else {
                    validate_ascii((char*)wstr->data, wstr->count);
                    write_bytes("\"", 1);
                    write_bytes(wstr->data, wstr->count);
                    write_bytes("\"", 1);
                }
            } break;

            case TypeKind::ByteArray: {
                write_byte_array((uint8_t*)value, size);
            } break;

            case TypeKind::ByteArrayCompressed: {
                auto buffer = scratch_buffer.advance(size);

                auto compressed_size = static_cast<mz_ulong>(scratch_buffer.remaining_size());
                auto result = mz_compress(buffer, &compressed_size, (const uint8_t*)value, static_cast<mz_ulong>(size));
                verify(result == MZ_OK);

                scratch_buffer.now += compressed_size;

                write_format("%X ", (uint32_t)size);
                auto output_size = base64_encode(buffer, compressed_size, (char*)scratch_buffer.now, scratch_buffer.end - scratch_buffer.now);
                write_bytes(scratch_buffer.now, output_size);
                
                write_byte_array(buffer, compressed_size);

                scratch_buffer.now = buffer;
            } break;

            case TypeKind::ByteArrayFixed: {
                verify(type->size == size);
                write_byte_array((uint8_t*)value, size);
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
                    case sizeof(float) : {
                        write_format("%f", *(float*)value);
                    } break;

                        case sizeof(double) : {
                            write_format("%f", *(double*)value);
                        } break;
                }
            } break;

            case TypeKind::Struct: {
                verify(type->size == size);
                auto struct_type = (const TypeStruct*)type;
                size_t offset = 0;
                for (int i = 0; i < struct_type->field_count; ++i) {
                    const auto& field = struct_type->fields[i];

                    write_bytes(field.name, strlen(field.name));
                    write_newline();

                    indent += 1;
                    write_indent();

                    write_type(field.type, (uint8_t*)value + offset, field.type->size);
                    offset += field.type->size;

                    indent -= 1;
                    if (i != struct_type->field_count - 1) {
                        write_newline();
                        write_indent();
                    }
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
                            write_bytes(field.name, strlen(field.name));
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
                if (VMAD_use_byte_array) {
                    write_byte_array((uint8_t*)value, size);
                    return;
                }

                BinaryReader r;
                r.start = (uint8_t*)value;
                r.now = r.start;
                r.end = r.start + size;

                #pragma pack(push, 1)
                struct Header {
                    int16_t version;
                    int16_t object_format;
                    uint16_t script_count;
                };

                struct PropertyObjectV1 {
                    FormID form_id;
                    int16_t alias;
                    uint16_t unused;
                };

                struct PropertyObjectV2 {
                    uint16_t unused;
                    int16_t alias;
                    FormID form_id;
                };
                #pragma pack(pop)

                auto header = r.advance<Header>();
                verify(header->version >= 2 && header->version <= 5);
                verify(header->object_format >= 1 && header->object_format <= 2);

                write_custom_field(true, "Version", header->version);
                write_custom_field(true, "Object Format", header->object_format);

                auto write_papyrus_object = [](TextRecordWriter& w, BinaryReader& r, uint16_t object_format, bool indent) {
                    verify(object_format == 2);
                    auto value = r.read<PropertyObjectV2>();
                    w.write_custom_field(true, "Form ID", value.form_id);
                    w.write_custom_field(true, "Alias", value.alias);
                    w.write_custom_field(indent, "Unused", value.unused);
                };

                for (int i = 0; i < header->script_count; ++i) {
                    begin_custom_struct("Script");
                    
                    write_custom_field(true, "Name", r.advance_wstring());

                    uint16_t property_count;
                    if (header->version >= 4) {
                        auto status = r.read<uint8_t>();
                        property_count = r.read<uint16_t>();
                        write_custom_field(property_count, "Status", status);
                    } else {
                        property_count = r.read<uint16_t>();
                    }

                    for (int prop_index = 0; prop_index < property_count; ++prop_index) {
                        begin_custom_struct("Property");

                        write_custom_field(true, "Name", r.advance_wstring());

                        auto property_type = r.read<PapyrusPropertyType>();
                        write_custom_field(true, "Type", property_type);
                        if (header->version >= 4) {
                            write_custom_field(true, "Status", r.read<uint8_t>());
                        }

                        switch (property_type) {
                            case PapyrusPropertyType::Object: {
                                write_papyrus_object(*this, r, header->object_format, false);
                            } break;

                            default: {
                                verify(false);
                            } break;
                        }

                        end_custom_struct(prop_index != property_count - 1);
                    }

                    end_custom_struct(i != header->script_count - 1);
                }

                if (r.now != r.end) {
                    write_newline();
                    write_indent();

                    // @NOTE: instead of using "current_record" we can make VMAD_INFO, VMAD_QUST, etc...
                    switch (current_record->type) {
                        case RecordType::INFO: {
                            verify(r.read<uint8_t>() == 2); // version?

                            auto flags = r.read<uint8_t>();
                            int fragment_count = 0;
                            if (flags == 1 || flags == 2) {
                                fragment_count = 1;
                            } else if (flags == 3) {
                                fragment_count = 2;
                            } else {
                                verify(false);
                            }
                            write_custom_field(true, "Fragment Flags", flags);
                            write_custom_field(true, "Fragment Script File Name", r.advance_wstring());

                            for (int i = 0; i < fragment_count; ++i) {
                                write_custom_field(true, "Unknown", r.read<uint8_t>());
                                write_custom_field(true, "Script Name", r.advance_wstring());
                                write_custom_field(i != fragment_count - 1, "Fragment Name", r.advance_wstring());
                            }
                        } break;

                        case RecordType::QUST: {
                            verify(r.read<uint8_t>() == 2); // version?

                            auto fragment_count = (int)r.read<uint16_t>();
                            write_custom_field(true, "File Name", r.advance_wstring());

                            for (int frag_index = 0; frag_index < fragment_count; ++frag_index) {
                                begin_custom_struct("Fragment");

                                write_custom_field(true, "Index", r.read<uint16_t>());
                                write_custom_field(true, "Unknown", r.read<uint16_t>());
                                write_custom_field(true, "Log Entry", r.read<uint32_t>());
                                write_custom_field(true, "Unknown", r.read<uint8_t>());
                                write_custom_field(true, "Script Name", r.advance_wstring());
                                write_custom_field(true, "Function Name", r.advance_wstring());

                                end_custom_struct(frag_index != fragment_count - 1);
                            };

                            auto alias_count = (int)r.read<uint16_t>();

                            for (int alias_index = 0; alias_index < alias_count; ++alias_index) {
                                begin_custom_struct("Alias");

                                write_papyrus_object(*this, r, header->object_format, true);
                                verify(r.read<uint16_t>() == header->version);
                                verify(r.read<uint16_t>() == header->object_format);

                                end_custom_struct(alias_index != alias_count - 1);
                            }
                        } break;

                        default: {
                            verify(false);
                        } break;
                    }
                }

                verify(r.now == r.end);
            } break;

            default: {
                verify(false);
            } break;
        }
    }

    void write_field(RecordDef* def, RecordField* field) {
        write_bytes(&field->type, 4);
        auto field_def = def->get_field_def(field->type);
        if (!field_def) {
            field_def = Record_Common.get_field_def(field->type);
        }

        if (field_def && field_def->comment) {
            write_bytes(" - ", 3);
            write_bytes(field_def->comment, strlen(field_def->comment));
        }

        write_newline();
        write_indent();

        auto now = (uint8_t*)field + sizeof(RecordField);
        if (field_def) {
            write_type(field_def->data_type, now, field->size);
        } else {
            write_byte_array(now, field->size);
        }
    }

    void begin_custom_struct(const char* header_name) {
        write_string(header_name);
        write_newline();
        ++indent;
        write_indent();
    }

    void end_custom_struct(bool do_indent = false) {
        --indent;
        if (do_indent) {
            write_newline();
            write_indent();
        }
    }

    void write_custom_field(bool indent_next_field, const char* field_name, const Type* type, const void* value, size_t size) {
        // @TODO: get rid of "indent_next_field", this makes code really ugly.

        write_string(field_name);
        write_newline();
        indent += 1;
        write_indent();
        write_type(type, value, size);
        indent -= 1;
        if (indent_next_field) {
            write_newline();
            write_indent();
        }
    }

    template<typename T>
    void write_custom_field(bool indent_next_field, const char* field_name, const T* value) {
        write_custom_field(indent_next_field, field_name, resolve_type<T>(), value, sizeof(T));
    }

    template<typename T>
    void write_custom_field(bool indent_next_field, const char* field_name, const T& value) {
        write_custom_field(indent_next_field, field_name, resolve_type<T>(), &value, sizeof(T));
    }
};

void esp_to_text(const wchar_t* esp_path, const wchar_t* text_path) {
    TextRecordWriter writer;
    writer.open(text_path);

    uint32_t size = 0;
    uint8_t* plugin = (uint8_t*)read_file(esp_path, &size);
    writer.write_records(plugin, plugin + size);

    writer.close();
}
