#pragma once

#include <Core/Common/Types.hpp>
#include <functional>
#include <map>
#include <vector>

namespace sw::core
{
	class TurnContext;

	// Отложенные эффекты: «в тик T выполнить эффект от источника S» (ARCHITECTURE.md, п. 4.4).
	// Выполняются в фазе Scheduled в порядке постановки. Периодические эффекты (горение)
	// сами ставят себя на следующий тик. Взрыв мины — отложенный эффект.
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

		// Только на будущие тики: at ≤ текущего — std::logic_error
		void schedule(Tick now, Tick at, UnitId source, Task task);

		// Забирает эффекты тика now в порядке постановки
		std::vector<Entry> takeDue(Tick now);

		bool empty() const
		{
			return _entries.empty();
		}

	private:
		// multimap хранит равные ключи в порядке вставки — это и есть FIFO одного тика
		std::multimap<Tick, Entry> _entries;
	};
}
