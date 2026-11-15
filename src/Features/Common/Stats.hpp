#pragma once

#include <Core/Units/IComponent.hpp>
#include <cstdint>

// Характеристики юнитов (SPECIFICATION.md, п. 9.1). Каждая — свой тип компонента: так
// действие объявляет, какую именно характеристику читает, и это проверяется при сборке.
// Значения знаковые (п. 1.4): будущий дебафф может опустить характеристику ниже нуля.
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

	// Дальность стрелка: стат, а не параметр действия, — бафф дальности меняет его,
	// не трогая действие (решение 7 спецификации)
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
