#include "esp_parser.hpp"
#include "os.hpp"
#include "array.hpp"

void EspParser::parse(const wchar_t* esp_path) {
    buffer = VirtualMemoryBuffer::alloc(1024 * 1024 * 128);

    uint32_t size = 0;
    uint8_t* plugin = (uint8_t*)read_file(esp_path, &size);

    process_records(plugin, plugin + size);
}

EspRecordBase* EspParser::process_record(const Record* record) {
    EspRecordBase* result_base;
    if (record->type == RecordType::GRUP) {
        auto result = buffer.advance<EspGrupRecord>();
        *result = EspGrupRecord();
        result_base = result;
    } else {
        auto result = buffer.advance<EspRecord>();
        *result = EspRecord();
        result_base = result;
    }

    verify(!record->current_user_id);
    verify(!record->last_user_id);

    result_base->type = record->type;
    result_base->flags = record->flags;

    if (record->type == RecordType::GRUP) {
        const auto grup_record = (const GrupRecord*)record;
        auto result = (EspGrupRecord*)result_base;

        result->label = grup_record->label;
        result->group_type = grup_record->group_type;
        result->unknown = grup_record->unknown;

        const uint8_t* now = (uint8_t*)grup_record + sizeof(GrupRecord);
        const uint8_t* end = now + grup_record->group_size - sizeof(GrupRecord);

        while (now < end) {
            const auto subrecord = (const Record*)now;
            result->records.push(process_record(subrecord));
            now += subrecord->data_size + (subrecord->type == RecordType::GRUP ? 0 : sizeof(Record));
        }
    } else {
        auto result = (EspRecord*)result_base;

        result->id = record->id;
        result->version = record->version;
        result->unknown = record->unknown;
        verify(record->version == 44);
        result->fields = Array<EspRecordField*>();

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
            result->fields.push(process_field(result, field));
            now += sizeof(RecordField) + field->size;
        }
    }

    return result_base;
}

EspRecordField* EspParser::process_field(EspRecord* record, const RecordField* field) {
    auto result = buffer.advance<EspRecordField>();
    memset(result, 0, sizeof(result));

    verify(record->type != RecordType::GRUP);

    result->type = field->type;
    result->data.count = field->size;
    result->data.data = (uint8_t*)field + sizeof(*field);

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
