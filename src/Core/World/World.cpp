#include "World.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace sw::core
{
	World::World(int64_t width, int64_t height) :
			_map(width, height),
			_query(_units, _map)
	{}

	Unit& World::place(Unit unit)
	{
		if (_units.find(unit.id()) != nullptr)
		{
			throw std::logic_error("World: unit id " + std::to_string(unit.id().value) + " is already present");
		}
		if (!_map.contains(unit.position()))
		{
			std::ostringstream message;
			message << "World: unit " << unit.id() << " is placed outside the map at " << unit.position();
			throw std::logic_error(message.str());
		}
		if (unit.occupiesCell())
		{
			_map.occupy(unit.position(), unit.id());
		}
		return _units.add(std::move(unit));
	}

	size_t World::removePending()
	{
		return _units.removePending(_map);
	}
}
