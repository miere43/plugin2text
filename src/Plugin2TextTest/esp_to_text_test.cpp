#include <CppUnitTest.h>
#include <esp_to_text.hpp>
#include "test_common.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EspToTextTest
{
    TEST_CLASS(RegressionTest) {
    public:
        TEST_METHOD(Test_ByteArrayRLE_ReadingPastProvidedBuffer) {
            TextRecordWriter writer;
            writer.init(ProgramOptions::None);
            defer(writer.dispose());

            const char zeros[15] = "\00\00\00\00\00\00\00\00\00\00\00\00\00\00";
            writer.write_type(&Type_ByteArrayRLE, zeros, 10); // pass only 10 zeros out of 15.
            writer.write_literal("\0"); // zero-terminate string.

            Assert::AreEqual("  ?*\n", (const char*)writer.output_buffer.start, L"reading past provided buffer");
        }
    };
}
