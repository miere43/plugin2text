#include <CppUnitTest.h>
#include <esp_to_text.hpp>
#include "test_common.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TextToEspTest
{
    TEST_CLASS(RegressionTest) {
public:
    TEST_METHOD(Test_ByteArrayCompressed_EmptyOutput) {
        // Bug: writing TypeKind::ByteArrayCompressed field inside compressed record results in field.size == 0.
        test_esps(
            ProgramOptions::None,
            L"../../../../test/regression/text_to_esp_byte_array_compressed.esm",
            L"../../../../test/regression/text_to_esp_byte_array_compressed_expect.txt",
            L"../../../../test/regression/text_to_esp_byte_array_compressed_expect.esm"
        );
    }
    };
}
