#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <Core/Common/Types.hpp>
#include <Core/Interaction/ITargetRule.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Core/Units/IComponent.hpp>
#include <cstdint>
#include <optional>

namespace sw::features
{
	struct RavenSpec
	{
		core::UnitId id;
		core::Position position;
		int64_t hp;
		int64_t agility;
	};

	class Flying final : public core::IComponent, public core::ITargetRule
	{
	public:
		std::optional<core::DistanceRange> adjust(
			const core::TargetSpec& incoming, core::DistanceRange range) const override;
	};

	core::SpawnRequest makeRaven(const RavenSpec& spec);
}
