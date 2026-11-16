#include "Scheduler.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sw::core
{
	void Scheduler::schedule(Tick now, Tick at, UnitId source, Task task)
	{
		if (at <= now)
		{
			throw std::logic_error(
				"Scheduler: effects can be scheduled only for future ticks (now " + std::to_string(now) + ", at "
				+ std::to_string(at) + ")");
		}
		if (!task)
		{
			throw std::logic_error("Scheduler: empty task");
		}
		_entries.emplace(at, Entry{at, source, std::move(task)});
	}

	std::vector<Scheduler::Entry> Scheduler::takeDue(Tick now)
	{
		std::vector<Entry> due;
		const auto [first, last] = _entries.equal_range(now);
		for (auto it = first; it != last; ++it)
		{
			due.push_back(std::move(it->second));
		}
		_entries.erase(first, last);
		return due;
	}
}
