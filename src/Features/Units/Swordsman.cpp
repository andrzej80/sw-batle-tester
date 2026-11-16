#include "Swordsman.hpp"

#include <Core/Actions/MarchStep.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/Attacks.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Common/UnitBuilder.hpp>
#include <Features/Common/ValueSource.hpp>

namespace sw::features
{
	core::SpawnRequest makeSwordsman(const SwordsmanSpec& spec)
	{
		UnitBuilder builder(spec.id, "Swordsman", spec.position);

		core::Diagnostics errors;
		requirePositive(errors, "hp", spec.hp);
		requirePositive(errors, "strength", spec.strength);
		if (!errors.empty())
		{
			return builder.reject(errors);
		}

		return builder.with<core::Health>(spec.hp)
			.with<Strength>(spec.strength)
			.with<core::Movement>(1)
			.action<MeleeAttack>(stat<Strength>)
			.action<core::MarchStep>()
			.spawnRequest();
	}
}
