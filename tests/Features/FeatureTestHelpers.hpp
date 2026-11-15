#pragma once

#include "Core/Simulation/SimulationTestHelpers.hpp"
#include "Core/World/WorldTestHelpers.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Common/UnitBuilder.hpp>
#include <string>
#include <vector>

namespace sw::test
{
	// Мир с симуляцией (SimArena) и журналом ударов
	struct FeatureArena : SimArena
	{
		FeatureArena(int64_t width, int64_t height) :
				SimArena(width, height)
		{
			bus.subscribe<core::events::UnitAttacked>(
				[this](const core::events::UnitAttacked& e)
				{
					attacks.push_back(
						std::to_string(e.attackerId.value) + "->" + std::to_string(e.targetId.value)
						+ " damage=" + std::to_string(e.damage) + " hp=" + std::to_string(e.targetHp));
				});
		}

		core::Unit& place(features::UnitBuilder& builder)
		{
			return world.place(builder.build());
		}

		core::Unit& place(features::UnitBuilder&& builder)
		{
			return place(builder);
		}

		// Неподвижная мишень с HP
		core::Unit& dummy(uint32_t id, core::Position position, bool occupiesCell = true, int64_t hp = 10)
		{
			features::UnitBuilder builder(core::UnitId{id}, "Dummy", position);
			builder.occupiesCell(occupiesCell).with<core::Health>(hp);
			return place(builder);
		}

		std::vector<std::string> attacks;
	};
}
