#include "esp_parser.hpp"
#include "os.hpp"
#include "array.hpp"
#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>

void EspParser::init(ProgramOptions options) {
    this->options = options;
    buffer = allocate_virtual_memory(1024 * 1024 * 128);
}

void EspParser::dispose() {
    // @TODO: free records (for tests)
    free_virtual_memory(&buffer);
}

void EspParser::parse(const StaticArray<uint8_t> data) {
    process_records(data.data, data.data + data.count);
}

RecordBase* EspParser::process_record(const RawRecord* record) {
    auto result_base = record->type == RecordType::GRUP
        ? static_cast<RecordBase*>(buffer.advance<GrupRecord>())
        : static_cast<RecordBase*>(buffer.advance<Record>());

    result_base->type = record->type;
    result_base->flags = record->flags;
    result_base->timestamp = record->timestamp;
    result_base->last_user_id = record->last_user_id;
    result_base->current_user_id = record->current_user_id;

    if (record->type == RecordType::GRUP) {
        const auto grup_record = (const RawGrupRecord*)record;
        auto result = (GrupRecord*)result_base;

        result->label = grup_record->label;
        result->group_type = grup_record->group_type;
        result->unknown = grup_record->unknown;

        const uint8_t* now = (uint8_t*)grup_record + sizeof(RawGrupRecord);
        const uint8_t* end = now + grup_record->group_size - sizeof(RawGrupRecord);

        while (now < end) {
            const auto subrecord = (const RawRecord*)now;
            result->records.push(process_record(subrecord));
            now += subrecord->data_size + (subrecord->type == RecordType::GRUP ? 0 : sizeof(RawRecord));
        }

        switch (result->group_type) {
            case RecordGroupType::CellPersistentChildren:
            case RecordGroupType::CellTemporaryChildren: {
                if (!is_bit_set(options, ProgramOptions::PreserveRecordOrder)) {
                    qsort(result->records.data, result->records.count, sizeof(result->records.data[0]), [](void const* aa, void const* bb) -> int {
                        const Record* a = *(const Record**)aa;
                        const Record* b = *(const Record**)bb;

                        verify(a->type != RecordType::GRUP);
                        verify(b->type != RecordType::GRUP);

                        verify(a->id.value != b->id.value);
                        return static_cast<int>(a->id.value) - static_cast<int>(b->id.value);
                    });
                }
            } break;
        }
    } else {
        auto result = (Record*)result_base;

        result->id = record->id;
        result->version = record->version;
        result->unknown = record->unknown;
        result->fields = Array<RecordField*>();

        const uint8_t* now;
        const uint8_t* end;
        if (record->is_compressed()) {
            uint32_t size = 0;
            now = uncompress_record((RawRecordCompressed*)record, &size);
            end = now + size;

            if (is_bit_set(options, ProgramOptions::DebugZLib)) {
                export_zlib_chunk((RawRecordCompressed*)record);
            }
        } else {
            now = (uint8_t*)record + sizeof(RawRecord);
            end = now + record->data_size;
        }

        while (now < end) {
            const auto field = (const RawRecordField*)now;
            result->fields.push(process_field(result, field));
            now += sizeof(RawRecordField) + field->size;
        }
    }

    return result_base;
}

RecordField* EspParser::process_field(Record* record, const RawRecordField* field) {
    auto result = buffer.advance<RecordField>();

    verify(record->type != RecordType::GRUP);

    result->type = field->type;
    result->data.count = field->size;
    result->data.data = (uint8_t*)field + sizeof(*field);

    return result;
}

void EspParser::process_records(const uint8_t* start, const uint8_t* end) {
    source_data_start = start;

    const uint8_t* now = start;
    while (now < end) {
        auto record = (RawRecord*)now;
        model.records.push(process_record(record));
        now += record->data_size + (record->type == RecordType::GRUP ? 0 : sizeof(RawRecord));
    }
}

uint8_t* EspParser::uncompress_record(const RawRecordCompressed* record, uint32_t* out_uncompressed_data_size) {
    uLongf uncompressed_data_size = record->uncompressed_data_size;
    uint8_t* compressed_data = (uint8_t*)(record + 1);

    auto uncompressed_data = buffer.advance(uncompressed_data_size);
    auto result = ::uncompress(uncompressed_data, &uncompressed_data_size, compressed_data, record->data_size - sizeof(record->uncompressed_data_size));
    verify(result == Z_OK);

    *out_uncompressed_data_size = uncompressed_data_size;
    return (uint8_t*)uncompressed_data;
}

void EspParser::export_zlib_chunk(const RawRecordCompressed* record) const {
    wchar_t file_path[2048];
    const auto count = swprintf_s(file_path, L"zlib_debug_%08X_%08X.bin", record->id.value, (uint32_t)((uint8_t*)record - source_data_start));
    verify(count > 0);

    write_file(file_path, { (uint8_t*)(record + 1), record->data_size - sizeof(record->uncompressed_data_size) });
    wprintf(L">>> exported %s\n", file_path);
}
