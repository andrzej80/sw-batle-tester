#pragma once

#include <Core/Common/Types.hpp>

namespace sw::core
{
	class EventBus;
	class TurnContext;
	class Unit;
	class World;

	class MovementService
	{
	public:
		MovementService(World& world, const EventBus& events);

		bool stepTo(const Unit& unit, Position cell, TurnContext& context);

		void addMarch(const Unit& unit, Position target);

		void finishMarch(const Unit& unit);

	private:
		World& _world;
		const EventBus& _events;
	};
}
