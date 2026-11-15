#pragma once

#include <cstdint>

namespace sw::core
{
	// Учёт активности за тик для правила «пустого тика» (ARCHITECTURE.md, п. 4.6).
	// Считает только Core: фичи ничего не сообщают сами.
	//
	// - markAction / markScheduled / markRemoval — выполненное действие, выполненный
	//   отложенный эффект, удаление в Cleanup; по ним решается, был ли тик пустым;
	// - markChange — вызывают изменяющие методы фасадов TurnContext; счётчик changes()
	//   не сбрасывается и служит проверке контракта IAction (false — мир не менялся).
	class ActivityTracker
	{
	public:
		void beginTick()
		{
			_somethingHappened = false;
		}

		void markAction()
		{
			_somethingHappened = true;
		}

		void markScheduled()
		{
			_somethingHappened = true;
		}

		void markRemoval()
		{
			_somethingHappened = true;
		}

		void markChange()
		{
			++_changes;
		}

		uint64_t changes() const
		{
			return _changes;
		}

		bool tickWasEmpty() const
		{
			return !_somethingHappened;
		}

	private:
		bool _somethingHappened = false;
		uint64_t _changes = 0;
	};
}
