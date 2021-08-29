#include <CppUnitTest.h>
#include <esp_parser.hpp>
#include <os.hpp>
#include <esp_to_text.hpp>
#include <text_to_esp.hpp>
#include "test_common.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CompareTest
{
    TEST_CLASS(CompareTest) {
private:
    void assert_same_array_content(const StaticArray<uint8_t>& expected, const StaticArray<uint8_t>& actual) {
        Assert::AreEqual(expected.count, actual.count, L"array size mismatch");
        Assert::IsTrue(memory_equals(expected.data, actual.data, expected.count), L"invalid array contents");
    }

    void test(const wchar_t* esp_path, const wchar_t* expect_txt_path, const wchar_t* expect_esp_path) {
        const auto empty_esp = read_file(esp_path);
        const auto empty_expect_txt = read_file(expect_txt_path);
        const auto empty_expect_esp = read_file(expect_esp_path);
        defer({
            delete[] empty_esp.data;
            delete[] empty_expect_txt.data;
            delete[] empty_expect_esp.data;
        });

        EspParser parser;
        parser.init();
        defer(parser.dispose());
        parser.parse(empty_esp);

        TextRecordWriter writer;
        writer.init();
        defer(writer.dispose());
        writer.write_records(parser.model.records);

        assert_same_array_content(empty_expect_txt, { writer.output_buffer.start, writer.output_buffer.size() });

        TextRecordReader reader;
        reader.init();
        defer(reader.dispose());
        reader.read_records((const char*)writer.output_buffer.start, (const char*)writer.output_buffer.now);

        assert_same_array_content(empty_expect_esp, { reader.buffer->start, reader.buffer->size() });
    }

public:
    TEST_METHOD(TestEmpty) {
        test(
            L"../../../../test/empty.esp",
            L"../../../../test/empty_expect.txt",
            L"../../../../test/empty_expect.esp"
        );
    }

    TEST_METHOD(TestWeap) {
        test(
            L"../../../../test/weap.esp",
            L"../../../../test/weap_expect.txt",
            L"../../../../test/weap_expect.esp"
        );
    }

    TEST_METHOD(TestInterior) {
        test(
            L"../../../../test/interior.esp",
            L"../../../../test/interior_expect.txt",
            L"../../../../test/interior_expect.esp"
        );
    }

    TEST_METHOD(TestNpc) {
        test(
            L"../../../../test/npc.esp",
            L"../../../../test/npc_expect.txt",
            L"../../../../test/npc_expect.esp"
        );
    }
    };
}
