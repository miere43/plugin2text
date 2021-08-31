#include "test_common.hpp"
#include <os.hpp>
#include <esp_parser.hpp>
#include <text_to_esp.hpp>
#include <esp_to_text.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void assert_same_array_content(const StaticArray<uint8_t>& expected, const StaticArray<uint8_t>& actual) {
    Assert::AreEqual(expected.count, actual.count, L"array size mismatch");
    Assert::IsTrue(memory_equals(expected.data, actual.data, expected.count), L"invalid array contents");
}

void test_esps(ProgramOptions options, const wchar_t* esp_path, const wchar_t* expect_txt_path, const wchar_t* expect_esp_path) {
    const auto empty_esp = read_file(esp_path);
    const auto empty_expect_txt = read_file(expect_txt_path);
    const auto empty_expect_esp = expect_esp_path ? read_file(expect_esp_path) : empty_esp;
    defer({
        delete[] empty_esp.data;
        delete[] empty_expect_txt.data;
        if (expect_esp_path) {
            delete[] empty_expect_esp.data;
        }
    });

    EspParser parser;
    parser.init(options);
    defer(parser.dispose());
    parser.parse(empty_esp);

    TextRecordWriter writer;
    writer.init(options);
    defer(writer.dispose());
    writer.write_records(parser.model.records);

    assert_same_array_content(empty_expect_txt, { writer.output_buffer.start, writer.output_buffer.size() });

    TextRecordReader reader;
    reader.init();
    defer(reader.dispose());
    reader.read_records((const char*)writer.output_buffer.start, (const char*)writer.output_buffer.now);

    assert_same_array_content(empty_expect_esp, { reader.buffer->start, reader.buffer->size() });
}