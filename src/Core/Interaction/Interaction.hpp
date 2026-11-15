#pragma once

#include "TargetSpec.hpp"

#include <memory>

namespace sw::core
{
	class EventBus;
	class Status;
	class Targeting;
	class Unit;
	class World;

	// Применение воздействия к цели (SPECIFICATION.md, п. 5.3). Единственное место, где
	// меняется Health, публикуются UnitAttacked / UnitHealed / UnitDied и накладываются
	// статусы. Каждое применение отмечается в ActivityTracker как изменение мира.
	class Interaction
	{
	public:
		Interaction(World& world, const Targeting& targeting, const EventBus& events);

		// Урон или лечение. Цель обязана быть выбрана через Targeting: живая, с Health.
		// 1. модификаторы цели по возрастанию priority(), при равенстве — в порядке
		//    добавления; amount < 0 после модификатора — std::logic_error;
		// 2. Damage: HP -= amount, UnitAttacked; HP впервые ≤ 0 — сразу UnitDied;
		// 3. Heal: HP = min(max, HP + amount), UnitHealed (в том числе с amount = 0).
		void apply(const Hit& hit);

		// Наложение статуса (spec.effect == Status). false — правила цели запретили
		// (иммунитет). Статус того же типа уже есть — решает он сам (Status::onReapplied).
		bool applyStatus(
			const Unit& source, const Unit& target, const TargetSpec& spec, std::unique_ptr<Status> status);

	private:
		World& _world;
		const Targeting& _targeting;
		const EventBus& _events;
	};
}
