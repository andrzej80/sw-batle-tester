#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <cstdint>

namespace sw::features
{
	struct HunterSpec
	{
		core::UnitId id;
		core::Position position;
		int64_t hp;
		int64_t agility;
		int64_t strength;
		int64_t range;
	};

	core::SpawnRequest makeHunter(const HunterSpec& spec);
}
