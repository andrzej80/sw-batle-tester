#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Units/Unit.hpp>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace sw::core
{
	class Map;

	class UnitStore
	{
	public:
		Unit& add(Unit unit);

		Unit* find(UnitId id);
		const Unit* find(UnitId id) const;

		// Изменяемый доступ к юниту, полученному по const-ссылке; юнит обязан лежать в
		// этом хранилище, иначе std::logic_error
		Unit& mutableOf(const Unit& unit);

		std::vector<UnitId> orderedIds() const;

		std::vector<const Unit*> ordered() const;

		size_t size() const
		{
			return _units.size();
		}

		size_t removePending(Map& map);

	private:
		std::vector<std::unique_ptr<Unit>> _units;
		std::unordered_map<UnitId, Unit*> _index;
	};
}
