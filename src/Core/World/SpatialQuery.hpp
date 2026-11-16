#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <Core/Common/Types.hpp>
#include <cstddef>
#include <vector>

namespace sw::core
{
	class Map;
	class Unit;
	class UnitStore;

	class SpatialQuery
	{
	public:
		SpatialQuery(const UnitStore& units, const Map& map);

		std::vector<const Unit*> unitsInRange(Position center, DistanceRange range) const;

		std::vector<const Unit*> neighbours(Position center) const;

		// Мёртвый юнит держит клетку до Cleanup
		bool isFreeFor(const Unit& unit, Position cell) const;

		const Unit* find(UnitId id) const;

		size_t aliveCount() const;

		const Map& map() const
		{
			return _map;
		}

	private:
		const UnitStore& _units;
		const Map& _map;
	};

	bool isAlive(const Unit& unit);
}
