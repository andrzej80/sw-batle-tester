#include "Simulation.hpp"

#include <Core/Common/TypeName.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <Core/World/World.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

namespace sw::core
{
	Simulation::Simulation(World& world, const EventBus& events, IRandom& random, Tick tickLimit) :
			_world(world),
			_tickLimit(tickLimit),
			_targeting(world.units()),
			_interaction(world, _targeting, events),
			_movement(world, events),
			_context(world, _targeting, _interaction, _movement, events, random)
	{
		if (tickLimit < 2)
		{
			throw std::logic_error("Simulation: tick limit must allow at least one turn (tick 2)");
		}
	}

	StopReason Simulation::run()
	{
		if (_world.query().aliveCount() <= 1)
		{
			return StopReason::OneOrNoUnitsLeft;
		}
		while (true)
		{
			if (const std::optional<StopReason> reason = runTick())
			{
				return *reason;
			}
		}
	}

	std::optional<StopReason> Simulation::runTick()
	{
		_world.advanceTick();
		_world.activity().beginTick();

		runScheduledPhase();
		runMainPhase();
		runCleanupPhase();

		if (_world.query().aliveCount() <= 1)
		{
			return StopReason::OneOrNoUnitsLeft;
		}
		if (_world.activity().tickWasEmpty() && _world.scheduler().empty())
		{
			return StopReason::NothingHappened;
		}
		if (_world.tick() >= _tickLimit)
		{
			return StopReason::TickLimit;
		}
		return std::nullopt;
	}

	void Simulation::runScheduledPhase()
	{
		for (Scheduler::Entry& entry : _world.scheduler().takeDue(_world.tick()))
		{
			entry.task(_context);
			_world.activity().markScheduled();
		}
	}

	void Simulation::runMainPhase()
	{
		// Снимок: юниты, созданные во время тика, ходят со следующего
		for (const UnitId id : _world.units().orderedIds())
		{
			Unit* unit = _world.units().find(id);
			if (unit == nullptr || !isAlive(*unit))
			{
				continue;
			}
			for (const std::unique_ptr<IAction>& action : unit->behaviour())
			{
				const uint64_t changesBefore = _world.activity().changes();
				if (action->tryExecute(*unit, _context))
				{
					_world.activity().markAction();
					break;
				}
				if (_world.activity().changes() != changesBefore)
				{
					throw std::logic_error(
						"IAction contract violated: " + typeName(*action) + " of unit "
						+ std::to_string(unit->id().value) + " returned false but changed the world");
				}
			}
		}
	}

	void Simulation::runCleanupPhase()
	{
		if (_world.removePending() > 0)
		{
			_world.activity().markRemoval();
		}
		const Tick now = _world.tick();
		for (const UnitId id : _world.units().orderedIds())
		{
			Unit* unit = _world.units().find(id);
			if (unit == nullptr)
			{
				continue;
			}
			std::vector<std::type_index> expired;
			for (const Status* status : unit->components().statuses())
			{
				if (status->isExpired(now))
				{
					expired.emplace_back(typeid(*status));
				}
			}
			for (const std::type_index type : expired)
			{
				unit->components().remove(type);
			}
		}
	}
}
