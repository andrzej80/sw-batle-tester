#include "SpatialQuery.hpp"

#include "UnitStore.hpp"

#include <Core/Map/Map.hpp>
#include <Core/Units/Unit.hpp>
#include <algorithm>

namespace sw::core
{
	bool isAlive(const Unit& unit)
	{
		return !unit.isDead() && !unit.isRemovalRequested();
	}

	SpatialQuery::SpatialQuery(const UnitStore& units, const Map& map) :
			_units(units),
			_map(map)
	{}

	std::vector<const Unit*> SpatialQuery::unitsInRange(Position center, DistanceRange range) const
	{
		std::vector<const Unit*> result;
		if (range.empty())
		{
			return result;
		}
		for (const Unit* unit : _units.ordered())
		{
			if (isAlive(*unit) && range.contains(chebyshev(center, unit->position())))
			{
				result.push_back(unit);
			}
		}
		return result;
	}

	std::vector<const Unit*> SpatialQuery::neighbours(Position center) const
	{
		return unitsInRange(center, DistanceRange{1, 1});
	}

	bool SpatialQuery::isFreeFor(const Unit& unit, Position cell) const
	{
		if (!_map.contains(cell))
		{
			return false;
		}
		return !unit.occupiesCell() || !_map.isOccupied(cell);
	}

	const Unit* SpatialQuery::find(UnitId id) const
	{
		return _units.find(id);
	}

	size_t SpatialQuery::aliveCount() const
	{
		const std::vector<const Unit*> units = _units.ordered();
		return static_cast<size_t>(
			std::count_if(units.begin(), units.end(), [](const Unit* unit) { return isAlive(*unit); }));
	}
}
