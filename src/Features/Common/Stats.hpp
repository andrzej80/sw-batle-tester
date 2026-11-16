#pragma once

#include <Core/Units/IComponent.hpp>
#include <cstdint>

namespace sw::features
{
	struct StatComponent : core::IComponent
	{
		explicit StatComponent(int64_t value) :
				value(value)
		{}

		int64_t value;
	};

	struct Strength final : StatComponent
	{
		using StatComponent::StatComponent;
	};

	struct Agility final : StatComponent
	{
		using StatComponent::StatComponent;
	};

	struct Range final : StatComponent
	{
		using StatComponent::StatComponent;
	};

	struct Power final : StatComponent
	{
		using StatComponent::StatComponent;
	};

	struct Spirit final : StatComponent
	{
		using StatComponent::StatComponent;
	};
}
