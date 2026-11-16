#include "TurnContext.hpp"

#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <Core/World/World.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace sw::core
{
	TurnContext::TurnContext(
		World& world,
		const Targeting& targeting,
		Interaction& interaction,
		MovementService& movement,
		const EventBus& events,
		IRandom& random) :
			_world(world),
			_targeting(targeting),
			_interaction(interaction),
			_movement(movement),
			_events(events),
			_random(random)
	{}

	Tick TurnContext::tick() const
	{
		return _world.tick();
	}

	const SpatialQuery& TurnContext::query() const
	{
		return _world.query();
	}

	void TurnContext::schedule(Tick at, UnitId source, Scheduler::Task task)
	{
		_world.scheduler().schedule(_world.tick(), at, source, std::move(task));
		markChange();
	}

	void TurnContext::requestRemoval(UnitId unit)
	{
		Unit* found = _world.units().find(unit);
		if (found == nullptr)
		{
			throw std::logic_error("TurnContext: requestRemoval of unknown unit " + std::to_string(unit.value));
		}
		// Юнит с Health уходит с поля только через смерть: отбор целей, правила цели и UNIT_DIED
		if (found->has<Health>())
		{
			throw std::logic_error(
				"TurnContext: requestRemoval of unit " + std::to_string(unit.value)
				+ " which has Health; such a unit leaves the map only by dying");
		}
		found->requestRemoval();
		markChange();
	}

	void TurnContext::markChange()
	{
		_world.activity().markChange();
	}
}
