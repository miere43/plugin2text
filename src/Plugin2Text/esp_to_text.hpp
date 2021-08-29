#pragma once
#include "esp_parser.hpp"
#include "typeinfo.hpp"

struct TextRecordWriter {
    Slice output_buffer;
    Slice scratch_buffer;

    int indent = 0;
    bool localized_strings = false; // @TODO: load value from TES4 record

    RecordType current_record_type = (RecordType)0; // Sometimes ESP deserialization depends on record type.

    void init();
    void dispose();

    void write_format(_Printf_format_string_ const char* format, ...);
    void write_byte_array(const uint8_t* data, size_t size);
    void write_indent();
    void write_newline();
    void write_string(const char* str);

    template<size_t N>
    void write_literal(const char(&data)[N]) {
        write_bytes(data, N - 1);
    }

    void write_bytes(const void* data, size_t size);
    void write_records(const Array<RecordBase*> records);
    void write_grup_record(const GrupRecord* record);
    RecordFlags write_flags(RecordFlags flags, const RecordDef* def);
    void write_record_timestamp(uint16_t timestamp);
    void write_record_unknown(uint32_t unknown);
    void write_record(const RecordBase* record_base);
    void write_papyrus_object(BinaryReader& r, const VMAD_Header* header);
    void write_papyrus_scripts(BinaryReader& r, const VMAD_Header* header, uint16_t script_count);
    void write_papyrus_info_record_fragment(BinaryReader& r, PapyrusFragmentFlags flags, const char* name, PapyrusFragmentFlags bit);
    void write_type(const Type* type, const void* value, size_t size);
    void write_field(const RecordField* field, const RecordFieldDef* field_def);
    void write_subrecord_fields(const RecordFieldDefSubrecord* field_def, StaticArray<RecordField*> fields);
    
    void begin_custom_struct(const char* header_name);
    void end_custom_struct();
    void write_custom_field(const char* field_name, const Type* type, const void* value, size_t size);

    template<typename T>
    void write_custom_field(const char* field_name, const T* value) {
        write_custom_field(field_name, resolve_type<T>(), value, sizeof(T));
    }

    template<typename T>
    void write_custom_field(const char* field_name, const T& value) {
        write_custom_field(field_name, resolve_type<T>(), &value, sizeof(T));
    }
};

void esp_to_text(const EspObjectModel& model, const wchar_t* text_path);
