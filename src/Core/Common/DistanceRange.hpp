#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>

namespace sw::core
{
	// Допустимые дистанции действия, включительно: [min, max].
	// min > max — пустой диапазон: ни одна дистанция не подходит.
	struct DistanceRange
	{
		int64_t min{};
		int64_t max{};

		bool contains(int64_t distance) const
		{
			return min <= distance && distance <= max;
		}

		bool empty() const
		{
			return min > max;
		}

		// Границы сдвигаются независимо: новый отрезок [min + minDelta, max + maxDelta]
		// пересекается с неотрицательными дистанциями [0, ∞). Пустое пересечение — nullopt:
		// досягаемости не осталось. Сдвиг никогда не создаёт досягаемость, которой не было
		// в сдвинутом отрезке (SPECIFICATION.md, п. 1.1 и 1.3).
		std::optional<DistanceRange> shifted(int64_t minDelta, int64_t maxDelta) const;

		friend bool operator==(const DistanceRange&, const DistanceRange&) = default;
	};

	std::ostream& operator<<(std::ostream& stream, const DistanceRange& range);
}
