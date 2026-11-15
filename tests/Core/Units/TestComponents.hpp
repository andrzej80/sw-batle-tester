#pragma once

// Компоненты и действия для тестов Core/Units: Core не знает конкретных статов и
// правил, поэтому тесты объявляют свои — так же, как это делают фичи.

#include <Core/Actions/IAction.hpp>
#include <Core/Interaction/IHitModifier.hpp>
#include <Core/Interaction/ITargetRule.hpp>
#include <Core/Units/ComponentRequirements.hpp>
#include <Core/Units/IComponent.hpp>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>

namespace sw::test
{
	struct Strength final : core::IComponent
	{
		explicit Strength(int64_t value) :
				value(value)
		{}

		int64_t value;
	};

	// Правило цели (как Flying у Ворона)
	struct RuleComponent final : core::IComponent, core::ITargetRule
	{
		std::optional<core::DistanceRange> adjust(const core::TargetSpec&, core::DistanceRange range) const override
		{
			return range;
		}
	};

	// Модификатор удара (как щит)
	struct ModifierComponent final : core::IComponent, core::IHitModifier
	{
		int priority() const override
		{
			return 0;
		}

		void modify(core::Hit&) override {}
	};

	// И правило, и модификатор
	struct RuleAndModifierComponent final : core::IComponent, core::ITargetRule, core::IHitModifier
	{
		std::optional<core::DistanceRange> adjust(const core::TargetSpec&, core::DistanceRange range) const override
		{
			return range;
		}

		int priority() const override
		{
			return 0;
		}

		void modify(core::Hit&) override {}
	};

	// Действие, которому нужны компоненты Required...
	template <class... Required>
	class RequiringAction final : public core::IAction
	{
	public:
		bool tryExecute(const core::Unit&, core::TurnContext&) override
		{
			return false;
		}

		void requirements(core::ComponentRequirements& requirements) const override
		{
			(requirements.require<Required>(), ...);
		}
	};

	// Текст std::logic_error, брошенного body; пусто — исключения не было
	inline std::string logicErrorMessage(const std::function<void()>& body)
	{
		try
		{
			body();
		}
		catch (const std::logic_error& error)
		{
			return error.what();
		}
		return {};
	}

	inline bool contains(const std::string& text, const std::string& fragment)
	{
		return text.find(fragment) != std::string::npos;
	}
}
