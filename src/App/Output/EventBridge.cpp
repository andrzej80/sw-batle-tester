#include "EventBridge.hpp"

namespace sw::app
{
	EventBridge::EventBridge(core::EventBus& bus, EventLog& log, std::function<core::Tick()> clock) :
			_bus(bus),
			_log(log),
			_clock(std::move(clock))
	{
		if (!_clock)
		{
			throw std::logic_error("EventBridge: empty clock");
		}
	}
}
