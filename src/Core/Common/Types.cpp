#include "Types.hpp"

#include <algorithm>
#include <ostream>

namespace sw::core
{
	namespace
	{
		int64_t absoluteDifference(int64_t a, int64_t b)
		{
			return a > b ? a - b : b - a;
		}
	}

	int64_t chebyshev(Position a, Position b)
	{
		return std::max(absoluteDifference(a.x, b.x), absoluteDifference(a.y, b.y));
	}

	std::ostream& operator<<(std::ostream& stream, UnitId id)
	{
		return stream << id.value;
	}

	std::ostream& operator<<(std::ostream& stream, Position position)
	{
		return stream << '(' << position.x << ',' << position.y << ')';
	}
}
