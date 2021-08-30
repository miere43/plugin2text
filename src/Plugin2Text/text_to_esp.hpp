#pragma once
#include "parseutils.hpp"
#include "typeinfo.hpp"

struct TextRecordReader {
    // Current buffer. If writing compressed data, then points to "compression_buffer", otherwise to "esp_buffer".
    Slice* buffer = nullptr;

    // Buffer for writing ESP.
    Slice esp_buffer;

    // Buffer for writing compressed data. Content from this buffer will be blitted into "esp_buffer" after compression.
    // @NOTE: It's possible to compress data inplace, but code for that is very annoying.
    // Looks like more trouble than worth.
    // Used as scratch buffer when uncompressing base64-encoded fields.
    Slice compression_buffer;

    bool inside_compressed_record = false;

    const char* start = nullptr;
    const char* now = nullptr;
    const char* end = nullptr;

    int indent = 0;

    RecordType current_record_type = (RecordType)0;

    void init();
    void dispose();

    void read_records(const char* start, const char* end);
    RecordGroupType expect_record_group_type();
    uint16_t read_record_timestamp();
    uint32_t read_record_unknown();
    void read_grup_record(RawGrupRecord* group);
    RecordFlags read_record_flags(RecordDef* def);
    RawRecord* read_record();
    FormID read_formid();
    void read_formid_line(Slice* slice);
    void read_byte_array(Slice* slice, size_t count);
    void read_papyrus_object(Slice* slice, const VMAD_Header* header);
    uint16_t read_papyrus_scripts(Slice* slice, const VMAD_Header* header);
    PapyrusFragmentFlags read_papyrus_info_record_fragment(Slice* slice, const char* fragment, PapyrusFragmentFlags flags);
    void read_string(Slice* slice);
    size_t read_type(Slice* slice, const Type* type);
    bool try_begin_custom_struct(const char* header_name);

    void end_custom_struct();
    void read_custom_field(Slice* slice, const char* field_name, const Type* type);

    template<typename T>
    T read_custom_field(const char* field_name) {
        T value;

        Slice slice;
        slice.start = (uint8_t*)&value;
        slice.now = slice.start;
        slice.end = slice.start + sizeof(T);

        read_custom_field(&slice, field_name, resolve_type<T>());

        return value;
    }

    template<typename T>
    void passthrough_custom_field(Slice* slice, const char* field_name) {
        read_custom_field(slice, field_name, resolve_type<T>());
    }

    void read_field(const RecordFieldDef* field_def);
    void read_subrecord_fields(const RecordFieldDefSubrecord* field_def);
    RecordType read_record_type();
    RecordFieldType read_record_field_type();
    bool expect(const char* string);
    int expect_int();
    int peek_indents();
    const char* peek_end_of_current_line();
    void expect_indent();
    void skip_to_next_line();
};


void text_to_esp(const wchar_t* text_path, const wchar_t* esp_path);
