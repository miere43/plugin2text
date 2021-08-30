#include "tes.hpp"
#include "common.hpp"
#include <string.h>
#include <stdlib.h>

bool RawRecord::is_compressed() const {
    return is_bit_set(flags, RecordFlags::Compressed);
}

const char* record_group_type_to_string(RecordGroupType type) {
    switch (type) {
        case RecordGroupType::Top:                    return "Top";
        case RecordGroupType::WorldChildren:          return "World";
        case RecordGroupType::InteriorCellBlock:      return "Interior Block";
        case RecordGroupType::InteriorCellSubBlock:   return "Interior Sub-Block";
        case RecordGroupType::ExteriorCellBlock:      return "Exterior";
        case RecordGroupType::ExteriorCellSubBlock:   return "Exterior Sub-Block";
        case RecordGroupType::CellChildren:           return "Cell";
        case RecordGroupType::TopicChildren:          return "Topic";
        case RecordGroupType::CellPersistentChildren: return "Persistent";
        case RecordGroupType::CellTemporaryChildren:  return "Temporary";
        default: verify(false); return nullptr;
    }
}

static const char* const ShortMonthNames[12]{
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec",
};

const char* month_to_short_string(int month) {
    verify(month >= 1 && month <= 12);
    return ShortMonthNames[month - 1];
}

int short_string_to_month(const char* str) {
    auto count = strlen(str);
    verify(count == 3);
    for (int i = 0; i < _countof(ShortMonthNames); ++i) {
        if (memory_equals(ShortMonthNames[i], str, 3)) {
            return i + 1;
        }
    }
    verify(false);
    return 0; 
}
