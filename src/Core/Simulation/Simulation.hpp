#pragma once

#include "MovementService.hpp"
#include "TurnContext.hpp"

#include <Core/Common/Types.hpp>
#include <Core/Interaction/Interaction.hpp>
#include <Core/Interaction/Targeting.hpp>
#include <cstdint>
#include <optional>

namespace sw::core
{
	class EventBus;
	class IRandom;
	class World;

	enum class StopReason : std::uint8_t
	{
		OneOrNoUnitsLeft,
		NothingHappened,
		TickLimit,
	};

	class Simulation
	{
	public:
		static constexpr Tick DefaultTickLimit = 30'000;

		Simulation(World& world, const EventBus& events, IRandom& random, Tick tickLimit = DefaultTickLimit);

		Simulation(const Simulation&) = delete;
		Simulation& operator=(const Simulation&) = delete;
		Simulation(Simulation&&) = delete;
		Simulation& operator=(Simulation&&) = delete;

		StopReason run();

		std::optional<StopReason> runTick();

		TurnContext& context()
		{
			return _context;
		}

	private:
		void runScheduledPhase();
		void runMainPhase();
		void runCleanupPhase();

		World& _world;
		Tick _tickLimit;
		Targeting _targeting;
		Interaction _interaction;
		MovementService _movement;
		TurnContext _context;
	};
}
