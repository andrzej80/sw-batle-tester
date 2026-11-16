#pragma once

#include "TargetSpec.hpp"

#include <memory>

namespace sw::core
{
	class EventBus;
	class Status;
	class Targeting;
	class Unit;
	class World;

	class Interaction
	{
	public:
		Interaction(World& world, const Targeting& targeting, const EventBus& events);

		void apply(const Hit& hit);

		bool applyStatus(
			const Unit& source, const Unit& target, const TargetSpec& spec, std::unique_ptr<Status> status);

	private:
		World& _world;
		const Targeting& _targeting;
		const EventBus& _events;
	};
}
