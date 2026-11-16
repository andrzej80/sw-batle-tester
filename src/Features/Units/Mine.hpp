#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <cstdint>

namespace sw::features
{
	struct MineSpec
	{
		core::UnitId id;
		core::Position position;
		int64_t power;
	};

	struct MineExploded
	{
		core::UnitId unitId;

		friend bool operator==(const MineExploded&, const MineExploded&) = default;
	};

	core::SpawnRequest makeMine(const MineSpec& spec);
}
