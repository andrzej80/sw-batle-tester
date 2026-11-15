#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sw::io
{
	// Строгий разбор поля uint32_t (SPECIFICATION.md, п. 8.1).
	//
	// operator>> для беззнаковых типов молча принимает минус и «хвосты»: проверено на
	// clang 15 с libstdc++ 13 и libc++ 15 — "-5" превращается в 4294967291, "5x" — в 5,
	// "1.5" — в 1, failbit не ставится. Поэтому здесь:
	// 1. в токене допускаются только цифры 0–9 — так отсекаются "-5", "+5", "5x", "1.5";
	// 2. значение получает std::from_chars; переполнение — ошибка.
	//
	// При ошибке возвращает nullopt и пишет причину в error (без имени поля).
	std::optional<uint32_t> parseUInt32(std::string_view token, std::string& error);
}
