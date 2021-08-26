#pragma once
#include "common.hpp"
#include "tes.hpp"

struct RecordField {
    RecordFieldType type = (RecordFieldType)0;
    StaticArray<uint8_t> data;
};

struct RecordBase {
    RecordType type;
    RecordFlags flags;
};

struct Record : RecordBase {
    FormID id;
    Array<RecordField*> fields;
    uint16_t version;
    uint16_t unknown;
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
};

struct EspObjectModel {
    Array<RecordBase*> records;
};

struct EspParser {
    VirtualMemoryBuffer buffer;

    EspObjectModel model;

    void parse(const wchar_t* esp_path);
private:
    RecordField* process_field(Record* record, const RawRecordField* field);
    RecordBase* process_record(const RawRecord* record);
    void process_records(const uint8_t* start, const uint8_t* end);
};