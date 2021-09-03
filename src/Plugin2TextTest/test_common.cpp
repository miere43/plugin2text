#include "test_common.hpp"
#include <os.hpp>
#include <esp_parser.hpp>
#include <text_to_esp.hpp>
#include <esp_to_text.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_MODULE_INITIALIZE(global_test_init) {
    void memory_init();
    memory_init();
}

void assert_same_array_content(const StaticArray<uint8_t>& expected, const StaticArray<uint8_t>& actual) {
    Assert::AreEqual(expected.count, actual.count, L"array size mismatch");
    Assert::IsTrue(memory_equals(expected.data, actual.data, expected.count), L"invalid array contents");
}

void test_esps(ProgramOptions options, const wchar_t* esp_path, const wchar_t* expect_txt_path, const wchar_t* expect_esp_path) {
    TEMP_SCOPE();

    const auto empty_esp = read_file(tmpalloc, esp_path);
    const auto empty_expect_txt = read_file(tmpalloc, expect_txt_path);
    const auto empty_expect_esp = expect_esp_path ? read_file(tmpalloc, expect_esp_path) : empty_esp;

    EspParser parser;
    parser.init(tmpalloc, options);
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