#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <cstdint>

namespace sw::features
{
	struct SwordsmanSpec
	{
		core::UnitId id;
		core::Position position;
		int64_t hp;
		int64_t strength;
	};

	core::SpawnRequest makeSwordsman(const SwordsmanSpec& spec);
}
