#pragma once
#include "common.hpp"
#include "tes.hpp"

struct EspRecordField {
    RecordFieldType type = (RecordFieldType)0;
    StaticArray<uint8_t> data;
};

struct EspRecord {
    RecordType type;
    RecordFlags flags;
    uint16_t version;
    uint32_t unknown;

    union {
        // valid if type == GRUP
        struct {
            Array<EspRecord*> records;
            RecordGroupType type;
            union {
                struct {
                    int16_t grid_y;
                    int16_t grid_x;
                };
                uint32_t label;
            };
        } group;

        // valid otherwise
        struct {
            Array<EspRecordField*> fields;
            FormID id;
            uint16_t version;
        } record;
    };

    inline EspRecord() {}
};

struct EspObjectModel {
    Array<EspRecord*> records;
};

struct EspParser {
    VirtualMemoryBuffer buffer;

    EspObjectModel model;

    void parse(const wchar_t* esp_path);
private:
    EspRecordField* process_field(EspRecord* record, const RecordField* field);
    EspRecord* process_record(const Record* record);
    void process_records(const uint8_t* start, const uint8_t* end);
};