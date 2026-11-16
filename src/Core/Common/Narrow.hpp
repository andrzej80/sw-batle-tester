#pragma once

#include <concepts>
#include <stdexcept>
#include <string>
#include <utility>

namespace sw::core
{
	template <std::integral To, std::integral From>
	To narrow(From value)
	{
		if (!std::in_range<To>(value))
		{
			throw std::logic_error("narrow: value " + std::to_string(value) + " is out of range of the target type");
		}
		return static_cast<To>(value);
	}
}
