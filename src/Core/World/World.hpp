#pragma once

#include "ActivityTracker.hpp"
#include "SpatialQuery.hpp"
#include "UnitStore.hpp"

#include <Core/Common/Types.hpp>
#include <Core/Map/Map.hpp>
#include <Core/Simulation/Scheduler.hpp>
#include <cstddef>

namespace sw::core
{
	// Мир — агрегат частей с одной ответственностью (ARCHITECTURE.md, п. 4.2). Логики
	// в нём нет, кроме связки «поставить юнита на карту» и «убрать удалённых с карты»,
	// где Map и UnitStore должны измениться вместе. Фазы тика выполняет Simulation.
	class World
	{
	public:
		World(int64_t width, int64_t height);

		World(const World&) = delete;
		World& operator=(const World&) = delete;
		World(World&&) = delete;
		World& operator=(World&&) = delete;

		Map& map()
		{
			return _map;
		}

		const Map& map() const
		{
			return _map;
		}

		UnitStore& units()
		{
			return _units;
		}

		const UnitStore& units() const
		{
			return _units;
		}

		const SpatialQuery& query() const
		{
			return _query;
		}

		ActivityTracker& activity()
		{
			return _activity;
		}

		const ActivityTracker& activity() const
		{
			return _activity;
		}

		Scheduler& scheduler()
		{
			return _scheduler;
		}

		const Scheduler& scheduler() const
		{
			return _scheduler;
		}

		// Текущий тик. Тик 1 — выполнение команд сценария; ходы начинаются с тика 2.
		// Номер тика нужен и выводу событий, поэтому он хранится в мире, а не в Simulation.
		Tick tick() const
		{
			return _tick;
		}

		void advanceTick()
		{
			++_tick;
		}

		// Ставит юнита на карту: клетка в карте, свободна (если юнит занимает клетку),
		// id не занят — иначе std::logic_error. Ошибки ввода проверяет SetupContext раньше.
		Unit& place(Unit unit);

		// Cleanup: удаляет мёртвых и помеченных, освобождает их клетки
		size_t removePending();

	private:
		Map _map;
		UnitStore _units;
		SpatialQuery _query;
		ActivityTracker _activity;
		Scheduler _scheduler;
		Tick _tick = 1;
	};
}
