#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <cstdint>

namespace sw::features
{
	struct HealerSpec
	{
		core::UnitId id;
		core::Position position;
		int64_t hp;
		int64_t spirit;
	};

	core::SpawnRequest makeHealer(const HealerSpec& spec);
}
