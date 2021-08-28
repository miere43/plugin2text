#include <CppUnitTest.h>
#include <esp_parser.hpp>
#include "test_common.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EspParserTest
{
    TEST_CLASS(EspParserTest) {
    public:
        TEST_METHOD(TestEmpty) {
            EspParser parser;
            parser.parse(L"../../../../test/empty.esp");

            Assert::AreEqual(parser.model.records.count, 1, L"invalid record count");
            auto record = static_cast<Record*>(parser.model.records[0]);
         
            Assert::AreEqual(record->type, RecordType::TES4, L"invalid record type");
            Assert::AreEqual(record->flags, RecordFlags::None, L"invalid record flags");
            Assert::AreEqual<uint32_t>(record->id.value, 0, L"invalid record id");
            Assert::AreEqual<uint16_t>(record->version, 44);
            Assert::AreEqual<uint16_t>(record->unknown, 0);

            Assert::AreEqual(record->fields.count, 7);
        }
    };
}
