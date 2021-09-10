#include "esp_parser.hpp"
#include "os.hpp"
#include "array.hpp"
#include <stdlib.h>
#include <zlib-ng.h>
#include <stdio.h>

void EspParser::init(Allocator& allocator, ProgramOptions options) {
    this->allocator = &allocator;
    this->options = options;
}

void EspParser::dispose() {
}

EspObjectModel EspParser::parse(const StaticArray<uint8_t> data) {
    source_data_start = data.data;

    const uint8_t* now = data.data;
    const uint8_t* end = data.data + data.count;
    
    EspObjectModel model;
    model.records.allocator = allocator;
    
    while (now < end) {
        auto record = (RawRecord*)now;
        model.records.push(process_record(record));
        now += record->data_size + (record->type == RecordType::GRUP ? 0 : sizeof(RawRecord));
    }

    return model;
}

RecordBase* EspParser::process_record(const RawRecord* record) {
    auto result_base = record->type == RecordType::GRUP
        ? static_cast<RecordBase*>(memnew(*allocator) GrupRecord(*allocator))
        : static_cast<RecordBase*>(memnew(*allocator) Record(*allocator));

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

        bool sort = false;
        switch (result->group_type) {
            case RecordGroupType::Top: {
                if (!is_bit_set(options, ProgramOptions::PreserveOrder) && (RecordType)result->label == RecordType::NPC_) {
                    sort = true;
                }
            } break;
            
            case RecordGroupType::CellPersistentChildren:
            case RecordGroupType::CellTemporaryChildren: {
                if (!is_bit_set(options, ProgramOptions::PreserveOrder)) {
                    sort = true;
                }
            } break;
        }

        if (sort) {
            qsort(result->records.data, result->records.count, sizeof(result->records.data[0]), [](void const* aa, void const* bb) -> int {
                const Record* a = *(const Record**)aa;
                const Record* b = *(const Record**)bb;

                verify(a->type != RecordType::GRUP);
                verify(b->type != RecordType::GRUP);

                verify(a->id.value != b->id.value);
                return static_cast<int>(a->id.value) - static_cast<int>(b->id.value);
            });
        }
    } else {
        auto result = (Record*)result_base;

        result->id = record->id;
        result->version = record->version;
        result->unknown = record->unknown;
        result->fields = Array<RecordField*>{ tmpalloc };

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
    auto result = memnew(*allocator) RecordField();

    verify(record->type != RecordType::GRUP);

    result->type = field->type;
    result->data.count = field->size;
    result->data.data = (uint8_t*)field + sizeof(*field);

    return result;
}

uint8_t* EspParser::uncompress_record(const RawRecordCompressed* record, uint32_t* out_uncompressed_data_size) {
    size_t uncompressed_data_size = record->uncompressed_data_size;
    uint8_t* compressed_data = (uint8_t*)(record + 1);

    auto uncompressed_data = (uint8_t*)memalloc(*allocator, uncompressed_data_size);
    auto result = ::zng_uncompress(uncompressed_data, &uncompressed_data_size, compressed_data, record->data_size - sizeof(record->uncompressed_data_size));
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

RecordField* Record::find_field(RecordFieldType type) const {
    for (const auto field : fields) {
        if (field->type == type) {
            return field;
        }
    }
    return nullptr;
}
