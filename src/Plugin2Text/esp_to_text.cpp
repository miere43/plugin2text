#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "esp_to_text.hpp"
#include "os.hpp"
#include "tes.hpp"
#include "common.hpp"
#include "typeinfo.hpp"
#include <stdio.h>

struct TextRecordWriter {
    HANDLE output_handle = 0;
    int indent = 0;
    bool localized_strings = false;

    void open(const wchar_t* path) {
        verify(!output_handle);
        output_handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        verify(output_handle != INVALID_HANDLE_VALUE);

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

    void write_bytes(const void* data, size_t size) {
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

    void write_record(Record* record) {
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

        const uint8_t* now = record->is_compressed() ? record->uncompress() : (uint8_t*)record + sizeof(Record);
        const uint8_t* end = now + record->data_size;

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

    void write_type(const Type* type, void* value, size_t size) {
        switch (type->kind) {
            case TypeKind::ZString: {
                if (size == 0) {
                    write_bytes("\"\"", 2);
                } else {
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

            case TypeKind::ByteArray: {
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
                
                uint32_t enum_value = 0;
                switch (size) {
                    case 1: enum_value = *(uint8_t*)value; break;
                    case 2: enum_value = *(uint16_t*)value; break;
                    case 4: enum_value = *(uint32_t*)value; break;
                    case 8: verify(false); break; //enum_value = *(uint64_t*)value; break;
                    default: verify(false); break;
                }
                
                auto enum_type = (const TypeEnum*)type;
                for (size_t i = 0; i < enum_type->field_count; ++i) {
                    const auto& field = enum_type->fields[i];
                    if (field.value == enum_value) {
                        write_bytes(field.name, strlen(field.name));
                        goto ok;
                    }
                }

                write_format("%u", enum_value);
                ok: break;
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
};

void esp_to_text(const wchar_t* esp_path, const wchar_t* text_path) {
    TextRecordWriter writer;
    writer.open(text_path);

    uint32_t size = 0;
    uint8_t* plugin = (uint8_t*)read_file(esp_path, &size);
    writer.write_records(plugin, plugin + size);

    writer.close();
}
