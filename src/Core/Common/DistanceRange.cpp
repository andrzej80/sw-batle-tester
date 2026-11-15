#include "DistanceRange.hpp"

#include <algorithm>
#include <ostream>

namespace sw::core
{
	std::optional<DistanceRange> DistanceRange::shifted(int64_t minDelta, int64_t maxDelta) const
	{
		const int64_t shiftedMin = std::max<int64_t>(min + minDelta, 0);
		const int64_t shiftedMax = max + maxDelta;
		if (shiftedMin > shiftedMax)
		{
			return std::nullopt;
		}
		return DistanceRange{shiftedMin, shiftedMax};
	}

	std::ostream& operator<<(std::ostream& stream, const DistanceRange& range)
	{
		return stream << '{' << range.min << ',' << range.max << '}';
	}
}
