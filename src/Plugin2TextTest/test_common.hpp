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
