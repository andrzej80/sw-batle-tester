#include "Core/Simulation/SimulationTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Random/IRandom.hpp>
#include <Core/Simulation/Scheduler.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using sw::core::Scheduler;
using sw::core::StdRandom;
using sw::core::TurnContext;
using sw::core::UnitId;

TEST(Scheduler_returns_due_tasks_in_order_of_scheduling)
{
	Scheduler scheduler;
	std::vector<std::string> order;
	scheduler.schedule(1, 3, UnitId{1}, [&](TurnContext&) { order.push_back("a"); });
	scheduler.schedule(1, 2, UnitId{2}, [&](TurnContext&) { order.push_back("early"); });
	scheduler.schedule(1, 3, UnitId{3}, [&](TurnContext&) { order.push_back("b"); });

	CHECK(scheduler.takeDue(1).empty());
	const std::vector<Scheduler::Entry> atTwo = scheduler.takeDue(2);
	CHECK_EQ(atTwo.size(), size_t{1});
	CHECK_EQ(atTwo[0].source, UnitId{2});

	const std::vector<Scheduler::Entry> atThree = scheduler.takeDue(3);
	CHECK_EQ(atThree.size(), size_t{2});
	CHECK_EQ(atThree[0].source, UnitId{1});
	CHECK_EQ(atThree[1].source, UnitId{3});
	CHECK(scheduler.empty());
}

TEST(Scheduler_accepts_only_future_ticks)
{
	Scheduler scheduler;
	CHECK_THROWS(scheduler.schedule(5, 5, UnitId{1}, [](TurnContext&) {}), std::logic_error);
	CHECK_THROWS(scheduler.schedule(5, 4, UnitId{1}, [](TurnContext&) {}), std::logic_error);
	CHECK_THROWS(scheduler.schedule(5, 6, UnitId{1}, Scheduler::Task{}), std::logic_error);
	CHECK(scheduler.empty());
}

TEST(StdRandom_is_reproducible_with_the_same_seed_and_stays_in_range)
{
	StdRandom first(42);
	StdRandom second(42);
	for (int i = 0; i < 100; ++i)
	{
		const size_t value = first.index(7);
		CHECK(value < 7);
		CHECK_EQ(value, second.index(7));
	}
	CHECK_EQ(first.index(1), size_t{0});
	CHECK_THROWS(static_cast<void>(first.index(0)), std::logic_error);
}
