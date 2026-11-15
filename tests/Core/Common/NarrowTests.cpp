#include "TestFramework/TestFramework.hpp"

#include <Core/Common/Narrow.hpp>
#include <cstdint>
#include <stdexcept>

using sw::core::narrow;

// Приёмка SPECIFICATION.md, п. 1.4
TEST(Narrow_rejects_values_outside_uint32)
{
	CHECK_THROWS(narrow<uint32_t>(int64_t{-1}), std::logic_error);
	CHECK_THROWS(narrow<uint32_t>(int64_t{4294967296}), std::logic_error);
}

TEST(Narrow_keeps_values_inside_uint32)
{
	CHECK_EQ(narrow<uint32_t>(int64_t{0}), uint32_t{0});
	CHECK_EQ(narrow<uint32_t>(int64_t{4294967295}), uint32_t{4294967295});
}

TEST(Narrow_works_for_signed_targets)
{
	CHECK_EQ(narrow<int32_t>(int64_t{-5}), int32_t{-5});
	CHECK_THROWS(narrow<int32_t>(int64_t{2147483648}), std::logic_error);
}
