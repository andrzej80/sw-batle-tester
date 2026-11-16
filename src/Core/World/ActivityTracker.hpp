#pragma once

#include <cstdint>

namespace sw::core
{
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
