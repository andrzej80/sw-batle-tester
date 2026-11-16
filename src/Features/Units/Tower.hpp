#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <cstdint>

namespace sw::features
{
	struct TowerSpec
	{
		core::UnitId id;
		core::Position position;
		int64_t hp;
		int64_t power;
	};

	core::SpawnRequest makeTower(const TowerSpec& spec);
}
