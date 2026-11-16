#pragma once

#include <Core/Common/TypeName.hpp>
#include <Core/Common/Types.hpp>
#include <Core/Events/EventBus.hpp>
#include <IO/System/EventLog.hpp>
#include <functional>
#include <stdexcept>
#include <typeindex>
#include <unordered_set>
#include <utility>

namespace sw::app
{
	class EventBridge
	{
	public:
		template <class E>
		using Output = std::function<void(const E&, EventLog&, core::Tick)>;

		EventBridge(core::EventBus& bus, EventLog& log, std::function<core::Tick()> clock);

		EventBridge(const EventBridge&) = delete;
		EventBridge& operator=(const EventBridge&) = delete;
		EventBridge(EventBridge&&) = delete;
		EventBridge& operator=(EventBridge&&) = delete;

		template <class E>
		EventBridge& on(Output<E> output)
		{
			if (!output)
			{
				throw std::logic_error("EventBridge: empty output for " + core::typeName<E>());
			}
			if (!_outputs.insert(std::type_index(typeid(E))).second)
			{
				throw std::logic_error("EventBridge: event " + core::typeName<E>() + " has two output adapters");
			}
			_bus.subscribe<E>([this, output = std::move(output)](const E& event) { output(event, _log, _clock()); });
			return *this;
		}

	private:
		core::EventBus& _bus;
		EventLog& _log;
		std::function<core::Tick()> _clock;
		std::unordered_set<std::type_index> _outputs;
	};
}
