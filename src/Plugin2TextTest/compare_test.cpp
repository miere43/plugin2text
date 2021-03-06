#include <CppUnitTest.h>
#include <esp_parser.hpp>
#include <os.hpp>
#include <esp_to_text.hpp>
#include <text_to_esp.hpp>
#include <xml.hpp>
#include "test_common.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CompareTest
{
    TEST_CLASS(CompareTest) {
public:
    TEST_METHOD(TestEmpty) {
        test_esps(
            ProgramOptions::None,
            L"../../../../test/empty.esp",
            L"../../../../test/empty_expect.txt",
            nullptr
        );
    }

    TEST_METHOD(TestWeap) {
        test_esps(
            ProgramOptions::None,
            L"../../../../test/weap.esp",
            L"../../../../test/weap_expect.txt",
            L"../../../../test/weap_expect.esp"
        );
    }

    TEST_METHOD(TestInterior) {
        test_esps(
            ProgramOptions::None,
            L"../../../../test/interior.esp",
            L"../../../../test/interior_expect.txt",
            L"../../../../test/interior_expect.esp"
        );
    }

    TEST_METHOD(TestNpc) {
        test_esps(
            ProgramOptions::ExportTimestamp,
            L"../../../../test/npc.esp",
            L"../../../../test/npc_expect.txt",
            L"../../../../test/npc_expect.esp"
        );
    }

    TEST_METHOD(TestMultilineString) {
        test_esps(
            ProgramOptions::None,
            L"../../../../test/multiline_string.esp",
            L"../../../../test/multiline_string_expect.txt",
            nullptr
        );
    }

    TEST_METHOD(TestVMAD) {
        test_esps(
            ProgramOptions::ExportTimestamp | ProgramOptions::PreserveOrder,
            L"../../../../test/vmad.esp",
            L"../../../../test/vmad_expect.txt",
            nullptr
        );
    }

    TEST_METHOD(TestDLVW) {
        test_esps(
            ProgramOptions::ExportTimestamp | ProgramOptions::PreserveOrder,
            L"../../../../test/dlvw.esp",
            L"../../../../test/dlvw_expect.txt",
            nullptr
        );
    }

    TEST_METHOD(TestDialogueViewFormatting) {
        const auto dlvw = read_file(tmpalloc, L"../../../../test/dlvw.xml");
        const auto dlvw_expect = read_file(tmpalloc, L"../../../../test/dlvw_expect.xml");

        XmlFormatter formatter;
        auto actual = formatter.format({ (char*)dlvw.data, (int)dlvw.count });

        assert_same_array_content(dlvw_expect, { (uint8_t*)actual.chars, (size_t)actual.count });
    }

    TEST_METHOD(TestCTDA) {
        test_esps(
            ProgramOptions::ExportTimestamp,
            L"../../../../test/ctda.esp",
            L"../../../../test/ctda_expect.txt",
            nullptr
        );
    }

    TEST_METHOD(TestXCLW) {
        test_esps(
            ProgramOptions::ExportTimestamp | ProgramOptions::PreserveJunk,
            L"../../../../test/xclw.esp",
            L"../../../../test/xclw_expect.txt",
            nullptr
        );
    }
    };
}
