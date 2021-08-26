#pragma once
#include "common.hpp"
#include "tes.hpp"

struct EspRecordField {
    RecordFieldType type = (RecordFieldType)0;
    StaticArray<uint8_t> data;
};

struct EspRecordBase {
    RecordType type;
    RecordFlags flags;
};

struct EspRecord : EspRecordBase {
    FormID id;
    Array<EspRecordField*> fields;
    uint16_t version;
    uint16_t unknown;
};

struct EspGrupRecord : EspRecordBase {
    RecordGroupType group_type;
    Array<EspRecordBase*> records;
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
    Array<EspRecordBase*> records;
};

struct EspParser {
    VirtualMemoryBuffer buffer;

    EspObjectModel model;

    void parse(const wchar_t* esp_path);
private:
    EspRecordField* process_field(EspRecord* record, const RecordField* field);
    EspRecordBase* process_record(const Record* record);
    void process_records(const uint8_t* start, const uint8_t* end);
};