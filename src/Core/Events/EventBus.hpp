#pragma once

#include <functional>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sw::core
{
	class EventBus
	{
	public:
		template <class E>
		void subscribe(std::function<void(const E&)> handler)
		{
			if (_publishDepth > 0)
			{
				throw std::logic_error("EventBus: subscribe() called while an event is being published");
			}
			if (!handler)
			{
				throw std::logic_error("EventBus: empty handler");
			}
			_handlers[std::type_index(typeid(E))].push_back([handler = std::move(handler)](const void* event)
															{ handler(*static_cast<const E*>(event)); });
		}

		template <class E>
		void publish(const E& event) const
		{
			const auto found = _handlers.find(std::type_index(typeid(E)));
			if (found == _handlers.end())
			{
				return;
			}
			const PublishGuard guard(_publishDepth);
			for (const ErasedHandler& handler : found->second)
			{
				handler(&event);
			}
		}

	private:
		using ErasedHandler = std::function<void(const void*)>;

		class PublishGuard
		{
		public:
			explicit PublishGuard(int& depth) :
					_depth(depth)
			{
				++_depth;
			}

			~PublishGuard()
			{
				--_depth;
			}

			PublishGuard(const PublishGuard&) = delete;
			PublishGuard& operator=(const PublishGuard&) = delete;

		private:
			int& _depth;
		};

		std::unordered_map<std::type_index, std::vector<ErasedHandler>> _handlers;
		mutable int _publishDepth = 0;
	};
}
