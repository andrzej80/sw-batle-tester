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

	// Запросы к миру только для чтения (SPECIFICATION.md, п. 4.3). Это единственный способ
	// для фич узнать, кто где стоит: изменять мир через него нельзя.
	//
	// «Живой» здесь — не мёртвый (HP > 0 или без Health) и не помеченный на удаление.
	// Мёртвые и помеченные не возвращаются, но мёртвый держит клетку до Cleanup.
	class SpatialQuery
	{
	public:
		SpatialQuery(const UnitStore& units, const Map& map);

		// Живые юниты на расстоянии из range от center, в порядке создания
		std::vector<const Unit*> unitsInRange(Position center, DistanceRange range) const;

		// Живые юниты на расстоянии 1
		std::vector<const Unit*> neighbours(Position center) const;

		// Может ли unit встать в cell: клетка в карте и, если unit занимает клетку, она
		// свободна (в том числе от мёртвого до уборки)
		bool isFreeFor(const Unit& unit, Position cell) const;

		const Unit* find(UnitId id) const;

		// Живые юниты на карте, включая мину
		size_t aliveCount() const;

		const Map& map() const
		{
			return _map;
		}

	private:
		const UnitStore& _units;
		const Map& _map;
	};

	// Юнит участвует в запросах: не мёртв и не помечен на удаление
	bool isAlive(const Unit& unit);
}
