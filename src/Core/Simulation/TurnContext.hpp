#pragma once

#include "MovementService.hpp"
#include "Scheduler.hpp"

#include <Core/Common/Types.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Interaction/Interaction.hpp>
#include <Core/Interaction/Targeting.hpp>
#include <Core/Random/IRandom.hpp>
#include <Core/World/SpatialQuery.hpp>

namespace sw::core
{
	class World;

	// Контекст хода — узкие фасады вместо доступа ко всему миру (ARCHITECTURE.md, п. 4.3).
	// Изменяемый World действиям недоступен: любое изменение мира идёт через фасад,
	// поэтому соблюдаются правила (занятость клеток, конвейер взаимодействий) и каждое
	// изменение отмечается в ActivityTracker — на этом держится проверка контракта IAction.
	class TurnContext
	{
	public:
		TurnContext(
			World& world,
			const Targeting& targeting,
			Interaction& interaction,
			MovementService& movement,
			const EventBus& events,
			IRandom& random);

		Tick tick() const;

		// Только чтение: кто где стоит
		const SpatialQuery& query() const;

		// Отбор целей с правилами цели
		const Targeting& targeting() const
		{
			return _targeting;
		}

		// Урон, лечение, наложение статуса
		Interaction& effects()
		{
			return _interaction;
		}

		// Перемещение с проверкой занятости и событиями
		MovementService& movement()
		{
			return _movement;
		}

		// Отложенный эффект на будущий тик
		void schedule(Tick at, UnitId source, Scheduler::Task task);

		// Удаление не через HP (мина после взрыва); юнит убирается в фазе Cleanup
		void requestRemoval(UnitId unit);

		IRandom& random()
		{
			return _random;
		}

		// Событие фичи: публикуется сразу и считается изменением мира
		template <class E>
		void publish(const E& event)
		{
			markChange();
			_events.publish(event);
		}

	private:
		void markChange();

		World& _world;
		const Targeting& _targeting;
		Interaction& _interaction;
		MovementService& _movement;
		const EventBus& _events;
		IRandom& _random;
	};
}
