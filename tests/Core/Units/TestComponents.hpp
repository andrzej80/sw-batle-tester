#pragma once

#include <Core/Actions/IAction.hpp>
#include <Core/Interaction/ITargetRule.hpp>
#include <Core/Units/ComponentRequirements.hpp>
#include <Core/Units/CoreComponents.hpp>
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

	struct RuleComponent final : core::IComponent, core::ITargetRule
	{
		std::optional<core::DistanceRange> adjust(const core::TargetSpec&, core::DistanceRange range) const override
		{
			return range;
		}
	};

	struct StatusComponent final : core::Status
	{
		StatusComponent() :
				Status(std::nullopt)
		{}
	};

	struct RuleAndStatusComponent final : core::Status, core::ITargetRule
	{
		RuleAndStatusComponent() :
				Status(std::nullopt)
		{}

		std::optional<core::DistanceRange> adjust(const core::TargetSpec&, core::DistanceRange range) const override
		{
			return range;
		}
	};

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
