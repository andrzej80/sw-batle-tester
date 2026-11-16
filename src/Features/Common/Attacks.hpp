#pragma once

#include "ValueSource.hpp"

#include <Core/Actions/IAction.hpp>
#include <functional>

namespace sw::features
{
	class MeleeAttack final : public core::IAction
	{
	public:
		explicit MeleeAttack(ValueSource damage);

		bool tryExecute(const core::Unit& self, core::TurnContext& context) override;
		void requirements(core::ComponentRequirements& requirements) const override;

	private:
		ValueSource _damage;
	};

	using ShotCondition = std::function<bool(const core::Unit& self, const core::TurnContext& context)>;

	class RangedAttack final : public core::IAction
	{
	public:
		RangedAttack(ValueSource damage, Distance distance, ShotCondition condition = {});

		bool tryExecute(const core::Unit& self, core::TurnContext& context) override;
		void requirements(core::ComponentRequirements& requirements) const override;

	private:
		ValueSource _damage;
		Distance _distance;
		ShotCondition _condition;
	};

	struct RequireNoAdjacentOccupying
	{
		bool operator()(const core::Unit& self, const core::TurnContext& context) const;
	};
}
