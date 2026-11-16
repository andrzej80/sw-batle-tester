#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>

namespace sw::core
{
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

		std::optional<DistanceRange> shifted(int64_t minDelta, int64_t maxDelta) const;

		friend bool operator==(const DistanceRange&, const DistanceRange&) = default;
	};

	std::ostream& operator<<(std::ostream& stream, const DistanceRange& range);
}
