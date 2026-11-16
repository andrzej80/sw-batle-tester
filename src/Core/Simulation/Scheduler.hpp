#pragma once

#include <Core/Common/Types.hpp>
#include <functional>
#include <map>
#include <vector>

namespace sw::core
{
	class TurnContext;

	class Scheduler
	{
	public:
		using Task = std::function<void(TurnContext&)>;

		struct Entry
		{
			Tick at;
			UnitId source;
			Task task;
		};

		void schedule(Tick now, Tick at, UnitId source, Task task);

		std::vector<Entry> takeDue(Tick now);

		bool empty() const
		{
			return _entries.empty();
		}

	private:
		// multimap хранит равные ключи в порядке вставки — это FIFO внутри тика
		std::multimap<Tick, Entry> _entries;
	};
}
