#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "text_to_esp.hpp"
#include "os.hpp"
#include "common.hpp"
#include "typeinfo.hpp"
#include <stdio.h>
#include <stdlib.h>
#include "base64.hpp"
#include <zlib.h>

struct TextRecordReader {
    // Current buffer. If writing compressed data, then points to "compression_buffer", otherwise to "esp_buffer".
    VirtualMemoryBuffer* buffer = nullptr;

    // Buffer for writing ESP.
    VirtualMemoryBuffer esp_buffer;

    // Buffer for writing compressed data. Content from this buffer will be blitted into "esp_buffer" after compression.
    // @NOTE: It's possible to compress data inplace, but code for that is very annoying.
    // Looks like more trouble than worth.
    // Used as scratch buffer when decompressing base64-encoded fields.
    VirtualMemoryBuffer compression_buffer;

    bool inside_compressed_record = false;

    const char* start = nullptr;
    const char* now = nullptr;
    const char* end = nullptr;

    int indent = 0;

    void open() {
        esp_buffer = VirtualMemoryBuffer::alloc(1024 * 1024 * 1024);
        compression_buffer = VirtualMemoryBuffer::alloc(1024 * 1024 * 32);
        buffer = &esp_buffer;
    }

    void close(const wchar_t* path) {
        auto output_handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        verify(output_handle != INVALID_HANDLE_VALUE);

        auto esp_size = (uint32_t)(buffer->now - buffer->start);
        DWORD written = 0;
        verify(WriteFile(output_handle, buffer->start, esp_size, &written, nullptr));
        verify(written == esp_size);

        CloseHandle(output_handle);
    }

    void read_records(const char* start, const char* end) {
        this->start = start;
        this->now = start;
        this->end = end;

        verify(expect("plugin2text version 1.00\n---\n"));

        while (now < end) {
            read_record();
        }
    }

    RecordGroupType expect_record_group_type() {
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

    uint16_t read_record_timestamp() {
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

    uint32_t read_record_unknown() {
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

    RawGrupRecord* read_grup_record() {
        auto record_start_offset = buffer->now;

        RawGrupRecord group;
        group.type = RecordType::GRUP;
    
        buffer->now += sizeof(RawGrupRecord);

        if (expect(" - ")) {
            auto line_end = peek_end_of_current_line();
            group.group_type = expect_record_group_type();

            switch (group.group_type) {
                case RecordGroupType::InteriorCellBlock:
                case RecordGroupType::InteriorCellSubBlock: {
                    verify(expect(" "));
                    group.label = expect_int();
                } break;
                
                case RecordGroupType::ExteriorCellBlock:
                case RecordGroupType::ExteriorCellSubBlock: {
                    verify(expect(" ("));
                    group.grid_x = expect_int();
                    verify(expect("; "));
                    group.grid_y = expect_int();
                    verify(expect(")"));
                } break;

                case RecordGroupType::CellChildren: 
                case RecordGroupType::CellPersistentChildren: 
                case RecordGroupType::CellTemporaryChildren: 
                case RecordGroupType::WorldChildren:
                case RecordGroupType::TopicChildren: {
                    // @TODO: It's possible to infer "group.value" by looking at previous record.
                    verify(expect(" "));
                    group.label = read_formid().value;
                } break;

                default: {
                    verify(false);
                } break;
            }
        } else {
            group.group_type = RecordGroupType::Top;
        }

        skip_to_next_line();

        group.timestamp = read_record_timestamp();
        group.unknown = read_record_unknown();

        indent += 1;
        while (true) {
            auto indents = peek_indents();
            if (indents == indent) {
                expect_indent();
                auto record = read_record();
                if (group.group_type == RecordGroupType::Top) {
                    if (record->type == RecordType::GRUP) {
                        if (!group.label) {
                            auto record_as_group = (RawGrupRecord*)record;
                            switch (record_as_group->group_type) {
                                case RecordGroupType::InteriorCellBlock: {
                                    group.label = fourcc("CELL");
                                } break;

                                case RecordGroupType::WorldChildren: {
                                    group.label = fourcc("WRLD");
                                } break;

                                default: {
                                    verify(false);
                                } break;
                            }
                        }
                    } else if (group.label == 0) {
                        group.label = (uint32_t)record->type;
                    } else {
                        verify(group.label == (uint32_t)record->type);
                    }
                }
            } else {
                verify(indents < indent);
                break;
            }
        }
        indent -= 1;

        // validate
        switch (group.group_type) {
            case RecordGroupType::Top: {
                verify(group.label);
            } break;

            // @TODO: validate other group_type's
        }

        group.group_size = (uint32_t)(buffer->now - record_start_offset);
        write_struct_at(record_start_offset, &group);

        return (RawGrupRecord*)record_start_offset;
    }

    RecordFlags read_record_flags(RecordDef* def) {
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

        indent += 1;
        while (true) {
            auto indents = peek_indents();
            if (indents == indent) {
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
            } else {
                verify(indents < indent);
                break;
            }
        }
        indent -= 1;

        return flags;
    }

    RawRecord* read_record() {
        auto record_start_offset = buffer->now;

        RawRecord record;
        record.type = read_record_type();

        if (record.type == RecordType::GRUP) {
            return (RawRecord*)read_grup_record();
        }

        record.version = 44; // @TODO
        buffer->now += sizeof(record);

        verify(expect(" "));
        record.id = read_formid();

        skip_to_next_line();

        record.timestamp = read_record_timestamp();
        {
            auto unknown = read_record_unknown();
            verify(unknown <= 0xffff);
            record.unknown = static_cast<uint16_t>(unknown);
        }

        auto def = get_record_def(record.type);
        record.flags = read_record_flags(def);
        if (!def) {
            def = &Record_Common;
        }

        const bool use_compression_buffer = record.is_compressed();
        if (use_compression_buffer) {
            verify(!inside_compressed_record);
            inside_compressed_record = true;
            buffer = &compression_buffer;
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

        if (use_compression_buffer) {
            constexpr int SkyrimZLibCompressionLevel = 7;

            auto uncompressed_data_size = static_cast<uLong>(compression_buffer.now - compression_buffer.start);
            verify(uncompressed_data_size > 0);

            auto compressed_size = static_cast<uLongf>(esp_buffer.end - esp_buffer.now); // remaining ESP size
            auto result = ::compress2(&record_start_offset[sizeof(record)] + sizeof(uint32_t), &compressed_size, buffer->start, uncompressed_data_size, SkyrimZLibCompressionLevel);
            verify(result == Z_OK);

            *(uint32_t*)&record_start_offset[sizeof(record)] = uncompressed_data_size;

            record.data_size = compressed_size + sizeof(uint32_t);
            buffer = &esp_buffer;
            buffer->now += record.data_size;
            compression_buffer.now = compression_buffer.start;

            inside_compressed_record = false;
        } else {
            record.data_size = (uint32_t)(buffer->now - record_start_offset - sizeof(record));
        }
        write_struct_at(record_start_offset, &record);

        return (RawRecord*)record_start_offset;
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

    FormID read_formid() {
        int nread = 0;
        FormID formid;
        int count = sscanf_s(now, "[%08X]%n", &formid.value, &nread);
        verify(count == 1);
        verify(nread == 1 + 8 + 1); // [DEADBEEF]
        return formid;
    }

    void read_formid_line() {
        auto line_end = peek_end_of_current_line();
        auto formid = read_formid();
        write_struct(&formid);
        now += 1 + 8 + 1; // [DEADBEEF]
        verify(now == line_end);
        now = line_end + 1; // +1 for '\n'.
    }

    void read_byte_array(size_t count) {
        const auto buffer = new uint8_t[count]; // @TODO: use scratch

        for (int i = 0; i < count; ++i) {
            uint8_t a = parse_hex_char(now[(i * 2) + 0]);
            uint8_t b = parse_hex_char(now[(i * 2) + 1]);
            buffer[i] = (a << 4) | b;
        }

        write_bytes(buffer, count);
        delete[] buffer;
    }

    void read_type(const Type* type) {
        // @TODO: Cleanup: there are too many statements like "now = line_end + 1".
        switch (type->kind) {
            case TypeKind::ByteArray: {
                const auto line_end = peek_end_of_current_line();
                const auto count = (line_end - now) / 2;
                verify(((line_end - now) % 2) == 0);

                read_byte_array(count);

                now = line_end + 1; // +1 for '\n'.
            } break;

            case TypeKind::ByteArrayCompressed: {
                const auto line_end = peek_end_of_current_line();
                const auto count = line_end - now;

                uint32_t decompressed_size = 0;
                int nread = 0;
                verify(1 == _snscanf_s(now, count, "%X%n", &decompressed_size, &nread));
                now += nread;
                verify(expect(" "));

                auto base64_buffer = compression_buffer.now;
                auto base64_size = (uLong)base64_decode(now, line_end - now, base64_buffer, compression_buffer.remaining_size());
                compression_buffer.advance(base64_size);
               
                auto result_size = (uLongf)decompressed_size;
                const auto decompressed_buffer = compression_buffer.advance(result_size);
                const auto result = ::uncompress(decompressed_buffer, &result_size, base64_buffer, base64_size);
                verify(result == Z_OK);

                write_bytes(decompressed_buffer, result_size);

                compression_buffer.now = base64_buffer;
                now = line_end + 1; // +1 for '\n'.
            } break;

            case TypeKind::ByteArrayFixed: {
                const auto line_end = peek_end_of_current_line();
                const auto count = (line_end - now) / 2;
                verify(((line_end - now) % 2) == 0);
                verify(count == type->size);

                read_byte_array(count);

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

                write_bytes(buffer, buffer_now - buffer);

                now = line_end + 1; // +1 for '\n'.
            } break;

            case TypeKind::ZString:
            case TypeKind::LString: {
                // @TODO: localized LString

                const auto line_end = peek_end_of_current_line();
                const auto count = (line_end - now) - 2;
                verify(now[0] == '"');
                write_bytes(now + 1, count);
                write_bytes("\0", 1);
                verify(now[count + 1] == '"');

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
                    if (field.type->kind == TypeKind::Constant) {
                        const auto constant_type = (const TypeConstant*)field.type;
                        write_bytes(constant_type->bytes, constant_type->size);
                        continue;
                    }
                    if (i != 0) {
                        expect_indent(); // @TODO: fix indent parsing
                    }
                    verify(expect(field.name));
                    verify(expect("\n"));
                    indent += 1;
                    expect_indent();
                    read_type(field.type);
                    indent -= 1;
                }
            } break;

            case TypeKind::FormID: {
                read_formid_line();
            } break;

            case TypeKind::FormIDArray: {
                read_formid_line();
                while (true) {
                    auto current_indent = peek_indents();
                    if (indent == current_indent) {
                        expect_indent();
                        read_formid_line();
                    } else {
                        verify(current_indent < indent);
                        break;
                    }
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

                        auto indents = peek_indents();
                        if (indents == indent) {
                            expect_indent();
                            continue;
                        } else {
                            verify(indents < indent);
                            break;
                        }
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

                switch (enum_type->size) {
                    case 1: {
                        uint8_t value = (uint8_t)result;
                        write_struct(&value);
                    } break;

                    case 2: {
                        uint16_t value = (uint16_t)result;
                        write_struct(&value);
                    } break;

                    case 4: {
                        uint32_t value = (uint32_t)result;
                        write_struct(&value);
                    } break;

                    case 8: {
                        uint64_t value = (uint64_t)result;
                        write_struct(&value);
                    } break;

                    default: {
                        verify(false);
                    } break;
                }
            } break;

            case TypeKind::Boolean: {
                auto line_end = peek_end_of_current_line();
                if (expect("True\n")) {
                    bool value = true;
                    write_struct(&value);
                } else if (expect("False\n")) {
                    bool value = false;
                    write_struct(&value);
                } else {
                    verify(false);
                }
            } break;

            case TypeKind::VMAD: {
                verify(false);
            } break;

            case TypeKind::Filter: {
                const auto filter_type = (const TypeFilter*)type;
                read_type(filter_type->inner_type);
            } break;

            case TypeKind::Vector3: {
                read_type(&Type_float);
                read_type(&Type_float);
                read_type(&Type_float);
            } break;

            default: {
                verify(false);
            } break;
        }
    }

    void read_field(const RecordDef* def, RawRecord* record) {
        auto field_start_offset = buffer->now;

        RawRecordField field;
        field.type = read_record_field_type();

        buffer->now += sizeof(field);
        auto field_def = def->get_field_def(field.type);
        if (!field_def) {
            field_def = Record_Common.get_field_def(field.type);
        }

        skip_to_next_line();

        indent += 1;

        expect_indent();
        read_type(field_def ? field_def->data_type : &Type_ByteArray);

        indent -= 1;

        field.size = (uint16_t)(buffer->now - field_start_offset - sizeof(field));
        write_struct_at(field_start_offset, &field);
    }

    RecordType read_record_type() {
        verify(now + 4 <= end);
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

    int expect_int() {
        int value = 0;
        int nread = 0;
        verify(1 == _snscanf_s(now, end - now, "%d%n", &value, &nread));
        now += nread;
        return value;
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

    template<typename T>
    void write_struct_at(uint8_t* pos, const T* data) {
        write_bytes_at(pos, data, sizeof(T));
    }

    void write_bytes(const void* data, size_t size) {
        write_bytes_at(buffer->now, data, size);
        buffer->now += size;
    }

    void write_bytes_at(uint8_t* pos, const void* data, size_t size) {
        verify(pos >= buffer->start);
        verify(pos + size <= buffer->end);
        memcpy(pos, data, size);
    }
};

void text_to_esp(const wchar_t* text_path, const wchar_t* esp_path) {
    TextRecordReader reader;
    reader.open();

    uint32_t size = 0;
    char* text = (char*)read_file(text_path, &size);
    reader.read_records(text, text + size);

    reader.close(esp_path);
}