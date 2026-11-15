#include "MovementService.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <Core/World/World.hpp>
#include <stdexcept>
#include <string>

namespace sw::core
{
	MovementService::MovementService(World& world, const EventBus& events) :
			_world(world),
			_events(events)
	{}

	Unit& MovementService::mutableUnit(const Unit& unit)
	{
		Unit* found = _world.units().find(unit.id());
		if (found != &unit)
		{
			throw std::logic_error("MovementService: unit " + std::to_string(unit.id().value) + " is not in the world");
		}
		return *found;
	}

	bool MovementService::stepTo(const Unit& unit, Position cell, TurnContext& context)
	{
		if (!unit.has<Movement>() || chebyshev(unit.position(), cell) != 1 || !_world.query().isFreeFor(unit, cell))
		{
			return false;
		}
		Unit& moving = mutableUnit(unit);
		if (moving.occupiesCell())
		{
			_world.map().release(moving.position(), moving.id());
			_world.map().occupy(cell, moving.id());
		}
		moving.setPosition(cell);
		_world.activity().markChange();
		_events.publish(events::UnitMoved{moving.id(), cell});
		_world.map().triggerEnter(cell, moving, context);
		return true;
	}

	void MovementService::addMarch(const Unit& unit, Position target)
	{
		Unit& marching = mutableUnit(unit);
		auto* movement = marching.tryGet<Movement>();
		if (movement == nullptr)
		{
			throw std::logic_error(
				"MovementService: unit " + std::to_string(unit.id().value) + " cannot move (no Movement)");
		}
		if (!_world.map().contains(target))
		{
			throw std::logic_error("MovementService: march target is outside the map");
		}
		movement->marches.push_back(target);
		_world.activity().markChange();
		if (movement->marches.size() == 1)
		{
			_events.publish(events::MarchStarted{marching.id(), marching.position(), target});
		}
	}

	void MovementService::finishMarch(const Unit& unit)
	{
		Unit& marching = mutableUnit(unit);
		auto& movement = marching.get<Movement>();
		if (movement.marches.empty() || movement.marches.front() != marching.position())
		{
			throw std::logic_error(
				"MovementService: unit " + std::to_string(unit.id().value) + " is not at its march target");
		}
		movement.marches.pop_front();
		_world.activity().markChange();
		_events.publish(events::MarchEnded{marching.id(), marching.position()});
		if (!movement.marches.empty())
		{
			_events.publish(events::MarchStarted{marching.id(), marching.position(), movement.marches.front()});
		}
	}
}
