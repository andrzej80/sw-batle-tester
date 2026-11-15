#include "ParseNumber.hpp"

#include <algorithm>
#include <charconv>
#include <system_error>

namespace sw::io
{
	std::optional<uint32_t> parseUInt32(std::string_view token, std::string& error)
	{
		const bool digitsOnly
			= !token.empty() && std::all_of(token.begin(), token.end(), [](char c) { return c >= '0' && c <= '9'; });
		if (!digitsOnly)
		{
			error = "expected non-negative integer, got \"" + std::string(token) + "\"";
			return std::nullopt;
		}

		uint32_t value = 0;
		const auto [end, code] = std::from_chars(token.data(), token.data() + token.size(), value);
		if (code == std::errc::result_out_of_range)
		{
			error = "value exceeds 4294967295, got " + std::string(token);
			return std::nullopt;
		}
		if (code != std::errc() || end != token.data() + token.size())
		{
			error = "expected non-negative integer, got \"" + std::string(token) + "\"";
			return std::nullopt;
		}
		return value;
	}
}
