#include "Core/Interaction/InteractionTestHelpers.hpp"
#include "Core/Simulation/SimulationTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <stdexcept>
#include <string>
#include <vector>

using sw::core::Hit;
using sw::core::Position;
using sw::core::StopReason;
using sw::core::Tick;
using sw::core::TurnContext;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::test;

namespace
{
	using Log = std::vector<std::string>;

	LambdaAction::Body alwaysDone()
	{
		return [](const Unit&, TurnContext&)
		{
			return true;
		};
	}
}

TEST(Simulation_with_one_unit_does_not_start)
{
	SimArena arena(10, 10);
	arena.spawn({.id = 1, .position = {0, 0}, .actions = {alwaysDone()}});

	CHECK(arena.simulation.run() == StopReason::OneOrNoUnitsLeft);
	CHECK_EQ(arena.world.tick(), Tick{1});
}

TEST(Simulation_stops_on_the_first_tick_where_nothing_happened)
{
	SimArena arena(10, 10);
	arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn({.id = 2, .position = {9, 9}});

	CHECK(arena.simulation.run() == StopReason::NothingHappened);
	CHECK_EQ(arena.world.tick(), Tick{2});
}

TEST(Simulation_units_act_in_creation_order)
{
	SimArena arena(10, 10);
	Log order;
	const auto record = [&](const char* name)
	{
		return [&order, name](const Unit&, TurnContext&)
		{
			order.emplace_back(name);
			return false;
		};
	};
	arena.spawn({.id = 5, .position = {0, 0}, .actions = {record("5")}});
	arena.spawn({.id = 2, .position = {2, 0}, .actions = {record("2")}});
	arena.spawn({.id = 9, .position = {4, 0}, .actions = {record("9")}});

	CHECK(arena.simulation.runTick() == StopReason::NothingHappened);
	CHECK(order == (Log{"5", "2", "9"}));
}

TEST(Simulation_first_successful_action_ends_the_turn)
{
	SimArena arena(10, 10);
	Log calls;
	arena.spawn(
		{.id = 1,
		 .position = {0, 0},
		 .actions
		 = {[&](const Unit&, TurnContext&)
			{
				calls.emplace_back("attack");
				return false;
			},
			[&](const Unit&, TurnContext&)
			{
				calls.emplace_back("special");
				return true;
			},
			[&](const Unit&, TurnContext&)
			{
				calls.emplace_back("never");
				return true;
			}}});
	arena.spawn({.id = 2, .position = {9, 9}});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(calls == (Log{"attack", "special"}));
}

TEST(Simulation_unit_killed_before_its_turn_does_not_act)
{
	SimArena arena(10, 10);
	Log acted;
	arena.spawn(
		{.id = 1,
		 .position = {0, 0},
		 .actions = {[](const Unit& self, TurnContext& context)
					 {
						 context.effects().apply(Hit{meleeAttack(), self.id(), UnitId{2}, 100});
						 return true;
					 }}});
	arena.spawn(
		{.id = 2,
		 .position = {1, 0},
		 .actions = {[&](const Unit&, TurnContext&)
					 {
						 acted.emplace_back("victim acted");
						 return true;
					 }}});
	arena.spawn({.id = 3, .position = {9, 9}});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(acted.empty());
	CHECK(arena.log == (Log{"[2] died 2"}));
	CHECK(arena.world.units().find(UnitId{2}) == nullptr);
	CHECK(!arena.world.map().isOccupied({1, 0}));
}

TEST(Simulation_stops_when_one_unit_is_left_after_cleanup)
{
	SimArena arena(10, 10);
	arena.spawn(
		{.id = 1,
		 .position = {0, 0},
		 .actions = {[](const Unit& self, TurnContext& context)
					 {
						 context.effects().apply(Hit{meleeAttack(), self.id(), UnitId{2}, 100});
						 return true;
					 }}});
	arena.spawn({.id = 2, .position = {1, 0}});

	CHECK(arena.simulation.run() == StopReason::OneOrNoUnitsLeft);
	CHECK_EQ(arena.world.tick(), Tick{2});
}

TEST(Simulation_detects_action_that_changed_the_world_but_returned_false)
{
	SimArena arena(10, 10);
	arena.spawn(
		{.id = 7,
		 .position = {0, 0},
		 .actions = {[](const Unit& self, TurnContext& context)
					 {
						 context.movement().stepTo(self, {1, 0}, context);
						 return false;
					 }}});
	arena.spawn({.id = 2, .position = {9, 9}});

	std::string message;
	try
	{
		static_cast<void>(arena.simulation.runTick());
	}
	catch (const std::logic_error& error)
	{
		message = error.what();
	}
	CHECK(message.find("IAction contract violated") != std::string::npos);
	CHECK(message.find("LambdaAction") != std::string::npos);
	CHECK(message.find("unit 7") != std::string::npos);
}

TEST(Simulation_stops_at_the_tick_limit)
{
	SimArena arena(10, 10, 5);
	arena.spawn({.id = 1, .position = {0, 0}, .actions = {alwaysDone()}});
	arena.spawn({.id = 2, .position = {9, 9}, .actions = {alwaysDone()}});

	CHECK(arena.simulation.run() == StopReason::TickLimit);
	CHECK_EQ(arena.world.tick(), Tick{5});
}

TEST(Simulation_runs_scheduled_effects_first_and_waits_for_pending_ones)
{
	SimArena arena(10, 10);
	Log timeline;
	bool armed = false;
	arena.spawn(
		{.id = 1,
		 .position = {5, 5},
		 .speed = std::nullopt,
		 .occupiesCell = false,
		 .hp = std::nullopt,
		 .actions = {[&](const Unit& self, TurnContext& context)
					 {
						 if (armed)
						 {
							 return false;
						 }
						 armed = true;
						 timeline.push_back("armed at " + std::to_string(context.tick()));
						 const UnitId mine = self.id();
						 context.schedule(
							 context.tick() + 3,
							 mine,
							 [&timeline, mine](TurnContext& later)
							 {
								 timeline.push_back("exploded at " + std::to_string(later.tick()));
								 later.requestRemoval(mine);
							 });
						 return true;
					 }}});
	arena.spawn(
		{.id = 2,
		 .position = {0, 0},
		 .actions = {[&](const Unit&, TurnContext& context)
					 {
						 timeline.push_back("main at " + std::to_string(context.tick()));
						 return false;
					 }}});
	arena.spawn({.id = 3, .position = {9, 9}});

	CHECK(arena.simulation.run() == StopReason::NothingHappened);
	CHECK(
		timeline
		== (Log{"armed at 2", "main at 2", "main at 3", "main at 4", "exploded at 5", "main at 5", "main at 6"}));
	CHECK_EQ(arena.world.tick(), Tick{6});
	CHECK(arena.world.units().find(UnitId{1}) == nullptr);
}

TEST(Simulation_removes_expired_statuses_in_cleanup)
{
	SimArena arena(10, 10);
	Unit& unit = arena.spawn({.id = 1, .position = {0, 0}, .actions = {alwaysDone()}});
	arena.spawn({.id = 2, .position = {9, 9}});
	unit.components().add<Burning>(Tick{3});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(unit.has<Burning>());
	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(!unit.has<Burning>());
}

TEST(Simulation_requires_a_tick_limit_that_allows_a_turn)
{
	sw::core::World world(10, 10);
	sw::core::EventBus bus;
	FirstRandom random;
	CHECK_THROWS(sw::core::Simulation(world, bus, random, 1), std::logic_error);
}
