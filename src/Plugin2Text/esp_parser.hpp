#pragma once
#include "common.hpp"
#include "tes.hpp"
#include "parseutils.hpp"

struct RecordField {
    RecordFieldType type = (RecordFieldType)0;
    StaticArray<uint8_t> data;
};

struct RecordBase {
    RecordType type = (RecordType)0;
    RecordFlags flags = RecordFlags::None;
    uint16_t timestamp = 0;
    uint8_t last_user_id = 0;
    uint8_t current_user_id = 0;
};

struct Record : RecordBase {
    FormID id;
    Array<RecordField*> fields;
    uint16_t version = 0;
    uint16_t unknown = 0;

    RecordField* find_field(RecordFieldType type) const;

    inline Record(Allocator& allocator) {
        fields.allocator = &allocator;
    }
};

struct GrupRecord : RecordBase {
    RecordGroupType group_type;
    Array<RecordBase*> records;
    union {
        struct {
            int16_t grid_y;
            int16_t grid_x;
        };
        uint32_t label;
    };
    uint32_t unknown;

    inline GrupRecord(Allocator& allocator) : group_type(RecordGroupType::Top), label(0), unknown(0) {
        records.allocator = &allocator;
    }
};

struct EspObjectModel {
    Array<RecordBase*> records;
};

struct EspParser {
    Allocator* allocator = &stdalloc;
    const uint8_t* source_data_start = nullptr;

    ProgramOptions options = ProgramOptions::None;

    void init(Allocator& allocator, ProgramOptions options);
    void dispose();

    EspObjectModel parse(const StaticArray<uint8_t> data);
private:
    RecordField* process_field(Record* record, const RawRecordField* field);
    RecordBase* process_record(const RawRecord* record);
    uint8_t* uncompress_record(const RawRecordCompressed* record, uint32_t* out_uncompressed_data_size);
    void export_zlib_chunk(const RawRecordCompressed* record) const;
};

template<typename Func>
void foreach_record(const Array<RecordBase*>& records, Func func) {
    for (const auto record : records) {
        if (record->type == RecordType::GRUP) {
            const auto group = (GrupRecord*)record;
            foreach_record(group->records, func);
        } else {
            func((Record*)record);
        }
    }
}