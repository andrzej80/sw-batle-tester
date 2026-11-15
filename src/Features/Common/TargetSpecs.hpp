#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <Core/Common/Tag.hpp>
#include <Core/Interaction/TargetSpec.hpp>
#include <utility>

// Именованные конструкторы TargetSpec (ARCHITECTURE.md, п. 4.9): четыре слова для
// действий, а оси Intent и Effect остаются раздельными для правил и модификаторов.
namespace sw::features
{
	inline core::TargetSpec attack(core::TagSet tags, core::DistanceRange range)
	{
		return {core::Intent::Hostile, core::Effect::Damage, std::move(tags), range};
	}

	inline core::TargetSpec heal(core::TagSet tags, core::DistanceRange range)
	{
		return {core::Intent::Beneficial, core::Effect::Heal, std::move(tags), range};
	}

	inline core::TargetSpec buff(core::TagSet tags, core::DistanceRange range)
	{
		return {core::Intent::Beneficial, core::Effect::Status, std::move(tags), range};
	}

	inline core::TargetSpec debuff(core::TagSet tags, core::DistanceRange range)
	{
		return {core::Intent::Hostile, core::Effect::Status, std::move(tags), range};
	}
}
