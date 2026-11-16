#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <Core/Common/Tag.hpp>
#include <Core/Common/Types.hpp>
#include <cstdint>
#include <utility>

namespace sw::core
{
	enum class Intent : std::uint8_t
	{
		Hostile,
		Beneficial,
	};

	enum class Effect : std::uint8_t
	{
		Damage,
		Heal,
		Status,
	};

	struct TargetSpec
	{
		TargetSpec(Intent intent, Effect effect, TagSet tags, DistanceRange range) :
				intent(intent),
				effect(effect),
				tags(std::move(tags)),
				range(range)
		{}

		Intent intent;
		Effect effect;
		TagSet tags;
		DistanceRange range;
	};

	struct Hit
	{
		TargetSpec spec;
		UnitId source;
		UnitId target;
		int64_t amount;
	};
}
