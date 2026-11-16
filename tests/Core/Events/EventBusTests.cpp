#include "TestFramework/TestFramework.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using sw::core::EventBus;
using sw::core::UnitId;
namespace events = sw::core::events;

namespace
{
	struct FeatureEvent
	{
		int value{};
	};
}

TEST(EventBus_calls_subscribers_in_subscription_order)
{
	EventBus bus;
	std::vector<std::string> calls;
	bus.subscribe<events::UnitDied>([&](const events::UnitDied& e)
									{ calls.push_back("first:" + std::to_string(e.unitId.value)); });
	bus.subscribe<events::UnitDied>([&](const events::UnitDied& e)
									{ calls.push_back("second:" + std::to_string(e.unitId.value)); });

	bus.publish(events::UnitDied{UnitId{7}});

	CHECK_EQ(calls.size(), size_t{2});
	CHECK_EQ(calls[0], std::string("first:7"));
	CHECK_EQ(calls[1], std::string("second:7"));
}

TEST(EventBus_delivers_only_to_subscribers_of_the_same_type)
{
	EventBus bus;
	int died = 0;
	int moved = 0;
	bus.subscribe<events::UnitDied>([&](const events::UnitDied&) { ++died; });
	bus.subscribe<events::UnitMoved>([&](const events::UnitMoved&) { ++moved; });

	bus.publish(events::UnitMoved{UnitId{1}, {2, 3}});

	CHECK_EQ(died, 0);
	CHECK_EQ(moved, 1);
}

TEST(EventBus_publishing_without_subscribers_is_not_an_error)
{
	const EventBus bus;
	bus.publish(events::MapCreated{10, 10});
	bus.publish(FeatureEvent{1});
	CHECK(true);
}

TEST(EventBus_passes_the_event_unchanged)
{
	EventBus bus;
	events::UnitAttacked received{};
	bus.subscribe<events::UnitAttacked>([&](const events::UnitAttacked& e) { received = e; });

	const events::UnitAttacked sent{UnitId{2}, UnitId{1}, 5, 0};
	bus.publish(sent);

	CHECK(received == sent);
}

TEST(EventBus_accepts_events_declared_outside_core)
{
	EventBus bus;
	int sum = 0;
	bus.subscribe<FeatureEvent>([&](const FeatureEvent& e) { sum += e.value; });

	bus.publish(FeatureEvent{3});
	bus.publish(FeatureEvent{4});

	CHECK_EQ(sum, 7);
}

TEST(EventBus_subscriber_may_publish_another_event)
{
	EventBus bus;
	std::vector<std::string> order;
	bus.subscribe<events::UnitAttacked>(
		[&](const events::UnitAttacked& e)
		{
			order.push_back("attacked");
			if (e.targetHp == 0)
			{
				bus.publish(events::UnitDied{e.targetId});
			}
		});
	bus.subscribe<events::UnitDied>([&](const events::UnitDied&) { order.push_back("died"); });

	bus.publish(events::UnitAttacked{UnitId{2}, UnitId{1}, 5, 0});

	CHECK_EQ(order.size(), size_t{2});
	CHECK_EQ(order[0], std::string("attacked"));
	CHECK_EQ(order[1], std::string("died"));
}

TEST(EventBus_subscribing_during_publish_is_a_logic_error)
{
	EventBus bus;
	bool rejected = false;
	bus.subscribe<events::UnitDied>(
		[&](const events::UnitDied&)
		{
			try
			{
				bus.subscribe<events::UnitDied>([](const events::UnitDied&) {});
			}
			catch (const std::logic_error&)
			{
				rejected = true;
			}
		});

	bus.publish(events::UnitDied{UnitId{1}});
	CHECK(rejected);

	bus.subscribe<events::UnitDied>([](const events::UnitDied&) {});
}

TEST(EventBus_rejects_empty_handler)
{
	EventBus bus;
	CHECK_THROWS(bus.subscribe<events::UnitDied>(std::function<void(const events::UnitDied&)>{}), std::logic_error);
}
