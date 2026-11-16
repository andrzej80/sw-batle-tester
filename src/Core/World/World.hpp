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

		Tick tick() const
		{
			return _tick;
		}

		void advanceTick()
		{
			++_tick;
		}

		Unit& place(Unit unit);

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
