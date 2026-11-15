#include "UnitStore.hpp"

#include <Core/Map/Map.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace sw::core
{
	Unit& UnitStore::add(Unit unit)
	{
		if (_index.contains(unit.id()))
		{
			throw std::logic_error("UnitStore: unit id " + std::to_string(unit.id().value) + " is already present");
		}
		auto& stored = _units.emplace_back(std::make_unique<Unit>(std::move(unit)));
		_index.emplace(stored->id(), stored.get());
		return *stored;
	}

	Unit* UnitStore::find(UnitId id)
	{
		const auto found = _index.find(id);
		return found == _index.end() ? nullptr : found->second;
	}

	const Unit* UnitStore::find(UnitId id) const
	{
		const auto found = _index.find(id);
		return found == _index.end() ? nullptr : found->second;
	}

	std::vector<UnitId> UnitStore::orderedIds() const
	{
		std::vector<UnitId> ids;
		ids.reserve(_units.size());
		for (const auto& unit : _units)
		{
			ids.push_back(unit->id());
		}
		return ids;
	}

	std::vector<const Unit*> UnitStore::ordered() const
	{
		std::vector<const Unit*> units;
		units.reserve(_units.size());
		for (const auto& unit : _units)
		{
			units.push_back(unit.get());
		}
		return units;
	}

	size_t UnitStore::removePending(Map& map)
	{
		size_t removed = 0;
		std::erase_if(
			_units,
			[&](const std::unique_ptr<Unit>& unit)
			{
				if (!unit->isDead() && !unit->isRemovalRequested())
				{
					return false;
				}
				if (unit->occupiesCell())
				{
					map.release(unit->position(), unit->id());
				}
				_index.erase(unit->id());
				++removed;
				return true;
			});
		return removed;
	}
}
