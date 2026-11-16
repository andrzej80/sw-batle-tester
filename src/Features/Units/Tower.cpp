#include "Tower.hpp"

#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/Attacks.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Common/UnitBuilder.hpp>
#include <Features/Common/ValueSource.hpp>

namespace sw::features
{
	namespace
	{
		constexpr int64_t MinShotDistance = 2;
		constexpr int64_t MaxShotDistance = 5;
	}

	core::SpawnRequest makeTower(const TowerSpec& spec)
	{
		UnitBuilder builder(spec.id, "Tower", spec.position);

		core::Diagnostics errors;
		requirePositive(errors, "hp", spec.hp);
		requirePositive(errors, "power", spec.power);
		if (!errors.empty())
		{
			return builder.reject(errors);
		}

		return builder.with<core::Health>(spec.hp)
			.with<Power>(spec.power)
			.action<RangedAttack>(stat<Power>, distance(MinShotDistance, MaxShotDistance))
			.spawnRequest();
	}
}
