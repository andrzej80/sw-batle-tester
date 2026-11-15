#pragma once

#include "ValueSource.hpp"

#include <Core/Actions/IAction.hpp>
#include <functional>

namespace sw::features
{
	// Удар по случайному юниту в соседней клетке: attack({melee, physical}, {1,1}).
	// Урон — значение damage в момент удара; отрицательное значение (будущий дебафф)
	// даёт удар с уроном 0. Целей нет — false.
	class MeleeAttack final : public core::IAction
	{
	public:
		explicit MeleeAttack(ValueSource damage);

		bool tryExecute(const core::Unit& self, core::TurnContext& context) override;
		void requirements(core::ComponentRequirements& requirements) const override;

	private:
		ValueSource _damage;
	};

	// Условие, при котором стрелок может стрелять
	using ShotCondition = std::function<bool(const core::Unit& self, const core::TurnContext& context)>;

	// Выстрел в случайного юнита на дистанции из distance: attack({ranged, physical}, …).
	// Дистанция вычисляется при каждом выстреле. Условие не выполнено или целей нет — false.
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

	// Стрелять можно, только если в соседних клетках нет живых юнитов, занимающих клетку.
	// Летающие, мины и мёртвые (до уборки) не мешают.
	struct RequireNoAdjacentOccupying
	{
		bool operator()(const core::Unit& self, const core::TurnContext& context) const;
	};
}
