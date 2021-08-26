#include "esp_parser.hpp"
#include "os.hpp"
#include "array.hpp"

void EspParser::parse(const wchar_t* esp_path) {
    uint32_t size = 0;
    uint8_t* plugin = (uint8_t*)read_file(esp_path, &size);

    buffer = VirtualMemoryBuffer::alloc(1024 * 1024 * 128);
}

EspRecord* EspParser::process_record(const Record* record) {
    auto result = buffer.advance<EspRecord>();
    memset(result, 0, sizeof(result));

    verify(record->version == 44);
    verify(!record->current_user_id);
    verify(!record->last_user_id);

    result->type = record->type;
    result->flags = record->flags;
    result->unknown = record->unknown;

    if (record->type == RecordType::GRUP) {
        const auto grup_record = (const GrupRecord*)record;
        
        result->group.label = grup_record->label;
        result->group.type = grup_record->group_type;
        result->group.records = Array<EspRecord*>();
        
        const uint8_t* now = (uint8_t*)grup_record + sizeof(GrupRecord);
        const uint8_t* end = now + grup_record->group_size - sizeof(GrupRecord);

        while (now < end) {
            const auto subrecord = (const Record*)now;
            result->group.records.push(process_record(subrecord));
            now += subrecord->data_size + (subrecord->type == RecordType::GRUP ? 0 : sizeof(Record));
        }
    } else {
        result->record.id = record->id;
        result->record.version = record->version;
        result->record.fields = Array<EspRecordField*>();

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

        while (now < end) {
            const auto field = (const RecordField*)now;
            result->record.fields.push(process_field(result, field));
            now += sizeof(RecordField) + field->size;
        }
    }

    return result;
}

EspRecordField* EspParser::process_field(EspRecord* record, const RecordField* field) {
    auto result = buffer.advance<EspRecordField>();
    memset(result, 0, sizeof(result));

    verify(record->type != RecordType::GRUP);

    result->type = field->type;
    result->data.count = field->size;
    result->data.data = (uint8_t*)field + sizeof(field);

    return result;
}

void EspParser::process_records(const uint8_t* start, const uint8_t* end) {
    const uint8_t* now = start;
    while (now < end) {
        auto record = (Record*)now;
        model.records.push(process_record(record));
        now += record->data_size + (record->type == RecordType::GRUP ? 0 : sizeof(Record));
    }
}
