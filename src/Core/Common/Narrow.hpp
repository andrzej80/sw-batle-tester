#pragma once

#include <concepts>
#include <stdexcept>
#include <string>
#include <utility>

namespace sw::core
{
	// Сужение целого с проверкой: значение вне диапазона To — std::logic_error.
	// Единственный способ перевести доменное int64_t в uint32_t формата вывода
	// (SPECIFICATION.md, п. 1.4). Выход за диапазон — баг, а не ошибка ввода.
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
