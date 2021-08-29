#pragma once
#include <CppUnitTest.h>
#include <tes.hpp>

namespace Microsoft{namespace VisualStudio{ namespace CppUnitTestFramework {

template<> inline std::wstring ToString<RecordType>(const RecordType& q) {
    RETURN_WIDE_STRING((uint32_t)q);
}

template<> inline std::wstring ToString<RecordFlags>(const RecordFlags& q) {
    RETURN_WIDE_STRING((uint32_t)q);
}

template<> inline std::wstring ToString<uint16_t>(const uint16_t& q) {
    RETURN_WIDE_STRING(q);
}

}}}

void assert_same_array_content(const StaticArray<uint8_t>& expected, const StaticArray<uint8_t>& actual);
void test_esps(ProgramOptions options, const wchar_t* esp_path, const wchar_t* expect_txt_path, const wchar_t* expect_esp_path);
