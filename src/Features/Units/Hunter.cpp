#include "Hunter.hpp"

#include <Core/Actions/MarchStep.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/Attacks.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Common/UnitBuilder.hpp>
#include <Features/Common/ValueSource.hpp>
#include <string>

namespace sw::features
{
	namespace
	{
		constexpr int64_t MinShotDistance = 2;
	}

	core::SpawnRequest makeHunter(const HunterSpec& spec)
	{
		UnitBuilder builder(spec.id, "Hunter", spec.position);

		core::Diagnostics errors;
		requirePositive(errors, "hp", spec.hp);
		requirePositive(errors, "agility", spec.agility);
		requirePositive(errors, "strength", spec.strength);
		if (spec.range < MinShotDistance)
		{
			errors.add("hunter.invalid_range", "range must be >= 2, got " + std::to_string(spec.range));
		}
		if (!errors.empty())
		{
			return builder.reject(errors);
		}

		return builder.with<core::Health>(spec.hp)
			.with<Agility>(spec.agility)
			.with<Strength>(spec.strength)
			.with<Range>(spec.range)
			.with<core::Movement>(1)
			.action<RangedAttack>(stat<Agility>, distance(MinShotDistance, stat<Range>), RequireNoAdjacentOccupying{})
			.action<MeleeAttack>(stat<Strength>)
			.action<core::MarchStep>()
			.spawnRequest();
	}
}
