#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sw::io
{
	// Не operator>>: для беззнаковых он молча принимает "-5" (4294967291) и "5x" (5).
	std::optional<uint32_t> parseUInt32(std::string_view token, std::string& error);
}
