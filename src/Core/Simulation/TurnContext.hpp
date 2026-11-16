#pragma once

#include "MovementService.hpp"
#include "Scheduler.hpp"

#include <Core/Common/Types.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Interaction/Interaction.hpp>
#include <Core/Interaction/Targeting.hpp>
#include <Core/Random/IRandom.hpp>
#include <Core/World/SpatialQuery.hpp>

namespace sw::core
{
	class World;

	class TurnContext
	{
	public:
		TurnContext(
			World& world,
			const Targeting& targeting,
			Interaction& interaction,
			MovementService& movement,
			const EventBus& events,
			IRandom& random);

		Tick tick() const;

		const SpatialQuery& query() const;

		const Targeting& targeting() const
		{
			return _targeting;
		}

		Interaction& effects()
		{
			return _interaction;
		}

		MovementService& movement()
		{
			return _movement;
		}

		void schedule(Tick at, UnitId source, Scheduler::Task task);

		void requestRemoval(UnitId unit);

		IRandom& random()
		{
			return _random;
		}

		template <class E>
		void publish(const E& event)
		{
			markChange();
			_events.publish(event);
		}

	private:
		void markChange();

		World& _world;
		const Targeting& _targeting;
		Interaction& _interaction;
		MovementService& _movement;
		const EventBus& _events;
		IRandom& _random;
	};
}
