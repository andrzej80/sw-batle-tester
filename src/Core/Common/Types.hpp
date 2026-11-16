#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>

namespace sw::core
{
	struct UnitId
	{
		uint32_t value{};

		friend bool operator==(UnitId, UnitId) = default;
		friend auto operator<=>(UnitId, UnitId) = default;
	};

	using Tick = uint64_t;

	struct Position
	{
		int64_t x{};
		int64_t y{};

		friend bool operator==(Position, Position) = default;
	};

	int64_t chebyshev(Position a, Position b);

	std::ostream& operator<<(std::ostream& stream, UnitId id);
	std::ostream& operator<<(std::ostream& stream, Position position);
}

template <>
struct std::hash<sw::core::UnitId>
{
	size_t operator()(sw::core::UnitId id) const noexcept
	{
		return std::hash<uint32_t>{}(id.value);
	}
};

template <>
struct std::hash<sw::core::Position>
{
	size_t operator()(sw::core::Position position) const noexcept
	{
		const size_t hx = std::hash<int64_t>{}(position.x);
		const size_t hy = std::hash<int64_t>{}(position.y);
		return hx ^ (hy + 0x9e3779b97f4a7c15ULL + (hx << 6) + (hx >> 2));
	}
};
