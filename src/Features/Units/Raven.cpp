#include "Raven.hpp"

#include <Core/Actions/MarchStep.hpp>
#include <Core/Interaction/TargetSpec.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/Attacks.hpp>
#include <Features/Common/CommonTags.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Common/UnitBuilder.hpp>
#include <Features/Common/ValueSource.hpp>

namespace sw::features
{
	namespace
	{
		constexpr int64_t Speed = 2;
	}

	std::optional<core::DistanceRange> Flying::adjust(const core::TargetSpec& incoming, core::DistanceRange range) const
	{
		if (incoming.intent != core::Intent::Hostile)
		{
			return range;
		}
		if (incoming.tags.has(tags::Melee))
		{
			return std::nullopt;
		}
		if (incoming.tags.has(tags::Ranged))
		{
			return range.shifted(-1, -1);
		}
		return range;
	}

	core::SpawnRequest makeRaven(const RavenSpec& spec)
	{
		UnitBuilder builder(spec.id, "Raven", spec.position);
		builder.occupiesCell(false);

		core::Diagnostics errors;
		requirePositive(errors, "hp", spec.hp);
		requirePositive(errors, "agility", spec.agility);
		if (!errors.empty())
		{
			return builder.reject(errors);
		}

		return builder.tag(tags::Flying)
			.with<core::Health>(spec.hp)
			.with<Agility>(spec.agility)
			.with<core::Movement>(Speed)
			.with<Flying>()
			.action<MeleeAttack>(stat<Agility>, distance(0, 1))
			.action<core::MarchStep>()
			.spawnRequest();
	}
}
