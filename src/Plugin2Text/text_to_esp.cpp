#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "text_to_esp.hpp"
#include "os.hpp"
#include "common.hpp"
#include "typeinfo.hpp"
#include <stdio.h>
#include <stdlib.h>

struct TextRecordReader {
    HANDLE output_handle = 0;
    uint32_t offset_in_file = 0;
    const char* start = nullptr;
    const char* now = nullptr;
    const char* end = nullptr;

    int indent = 0;

    void open(const wchar_t* path) {
        verify(!output_handle);
        output_handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        verify(output_handle != INVALID_HANDLE_VALUE);
    }

    void close() {
        verify(output_handle);
        CloseHandle(output_handle);
        output_handle = 0;
    }

    void read_records(const char* start, const char* end) {
        this->start = start;
        this->now = start;
        this->end = end;

        verify(expect("plugin2text version 1.00\n---\n"));

        while (now < end) {
            read_record();
            break;
        }
    }

    void read_record() {
        Record record;
        record.type = read_record_type();
        record.data_size = 0;
        record.flags = (RecordFlags)0;
        record.id.value = 0;
        record.unused0 = 0;
        record.unused1 = 0;
        write_struct(&record);

        skip_to_next_line();

        auto def = get_record_def(record.type);
        if (!def) {
            def = &Record_Common;
        }

        indent += 1;
        while (true) {
            auto indents = peek_indents();
            if (indents == indent) {
                expect_indent();
                read_field(def, &record);
            } else {
                verify(indents < indent);
                break;
            }
        }
        indent -= 1;
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

    void read_type(const Type* type) {
        switch (type->kind) {
            case TypeKind::ByteArray: {
                const auto line_end = peek_end_of_current_line();
                const auto count = (line_end - now) / 2;
                verify(((line_end - now) % 2) == 0);
            
                const auto buffer = new uint8_t[count];
                
                for (int i = 0; i < count; ++i) {
                    uint8_t a = parse_hex_char(now[(i * 2) + 0]);
                    uint8_t b = parse_hex_char(now[(i * 2) + 1]);
                    buffer[i] = (a << 4) | b;
                }
                
                write_bytes(buffer, count);
                delete[] buffer;

                now = line_end + 1; // +1 for '\n'.
            } break;
            
            case TypeKind::ZString:
            case TypeKind::LString: {
                // @TODO: localized LString

                const auto line_end = peek_end_of_current_line();
                const auto count = (line_end - now) - 2;
                expect("\"");
                write_bytes(now, count);
                write_bytes("\0", 1);
                expect("\"");

                now = line_end + 1; // +1 for '\n'.
            } break;

            case TypeKind::Float: {
                auto line_end = peek_end_of_current_line();
                int nread = 0;
                
                switch (type->size) {
                    case sizeof(float): {
                        float value = 0;
                        int count = sscanf_s(now, "%f%n", &value, &nread);
                        verify(count == 1);
                        verify(nread == line_end - now);
                        write_struct(&value);
                    } break;

                    case sizeof(double): {
                        double value = 0;
                        int count = sscanf_s(now, "%lf%n", &value, &nread);
                        verify(count == 1);
                        verify(nread == line_end - now);
                        write_struct(&value);
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

                switch (integer_type->size) {
                    case 1: {
                        uint8_t value8 = (uint8_t)value;
                        write_struct(&value8);
                    } break;

                    case 2: {
                        uint16_t value16 = (uint16_t)value;
                        write_struct(&value16);
                    } break;

                    case 4: {
                        uint32_t value32 = (uint32_t)value;
                        write_struct(&value32);
                    } break;

                    case 8: {
                        write_struct(&value);
                    } break;
                }

                now = line_end + 1; // +1 for '\n'.
            } break;

            case TypeKind::Struct: {
                const auto struct_type = (const TypeStruct*)type;
                for (int i = 0; i < struct_type->field_count; ++i) {
                    const auto& field = struct_type->fields[i];
                    verify(expect(field.name));
                    verify(expect("\n"));
                    indent += 1;
                    expect_indent();
                    read_type(field.type);
                    indent -= 1;

                    if (i != struct_type->field_count - 1) {
                        expect_indent();
                    }
                }
            } break;

            default: {
                verify(false);
            } break;
        }
    }

    void read_field(const RecordDef* def, Record* record) {
        RecordField field;
        field.type = read_record_field_type();
        field.size = 0;
        write_struct(&field);

        auto field_def = def->get_field_def(field.type);

        skip_to_next_line();

        indent += 1;

        expect_indent();
        read_type(field_def ? field_def->data_type : &Type_ByteArray);

        indent -= 1;
    }

    RecordType read_record_type() {
        verify(now + 4 < end);
        auto result = *(RecordType*)now;
        now += 4;
        return result;
    }

    RecordFieldType read_record_field_type() {
        verify(now + 4 < end);
        auto result = *(RecordFieldType*)now;
        now += 4;
        return result;
    }

    bool expect(const char* string) {
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

    int peek_indents() {
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

    const char* peek_end_of_current_line() {
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

    void expect_indent() {
        for (int i = 0; i < indent; ++i) {
            verify(expect("  "));
        }
        verify(now >= end || now[0] != ' ');
    }

    void skip_to_next_line() {
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

    template<typename T>
    void write_struct(const T* data) {
        write_bytes(data, sizeof(T));
    }

    void write_bytes(const void* data, size_t size) {
        DWORD written = 0;
        verify(WriteFile(output_handle, data, (uint32_t)size, &written, nullptr));
        verify(written == size);
        offset_in_file += written;
    }
};

void text_to_esp(const wchar_t* text_path, const wchar_t* esp_path) {
    TextRecordReader reader;
    reader.open(esp_path);

    uint32_t size = 0;
    char* text = (char*)read_file(text_path, &size);
    reader.read_records(text, text + size);

    reader.close();
}