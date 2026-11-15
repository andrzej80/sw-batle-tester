#pragma once

#include <Core/Common/Types.hpp>

namespace sw::core
{
	class EventBus;
	class TurnContext;
	class Unit;
	class World;

	// Единственный способ изменить позицию юнита и его очередь маршей
	// (ARCHITECTURE.md, п. 4.7). Держит занятость клеток согласованной с позициями,
	// публикует UnitMoved / MarchStarted / MarchEnded и отмечает изменения мира.
	class MovementService
	{
	public:
		MovementService(World& world, const EventBus& events);

		// Шаг в соседнюю клетку. false (и ничего не меняется) — у юнита нет Movement
		// (статичен при любых условиях), клетка вне карты, не соседняя или занята для
		// этого юнита. При успехе: клетки освобождаются и занимаются (для occupiesCell),
		// позиция меняется, публикуется UnitMoved, срабатывают триггеры клетки.
		bool stepTo(const Unit& unit, Position cell, TurnContext& context);

		// Ставит марш в очередь. Если очередь была пуста, марш становится активным —
		// публикуется MarchStarted. Нет Movement или цель вне карты — std::logic_error
		// (ошибки ввода ловит SetupContext раньше).
		void addMarch(const Unit& unit, Position target);

		// Юнит стоит в цели активного марша: MarchEnded, следующий марш из очереди
		// становится активным — MarchStarted, в том же тике.
		void finishMarch(const Unit& unit);

	private:
		Unit& mutableUnit(const Unit& unit);

		World& _world;
		const EventBus& _events;
	};
}
