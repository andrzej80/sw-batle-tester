#include "Core/Simulation/SimulationTestHelpers.hpp"
#include "Core/World/WorldTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <stdexcept>

using sw::core::Position;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::test;

namespace
{
	using Log = std::vector<std::string>;
}

TEST(MovementService_steps_into_a_free_neighbour_cell)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});

	CHECK(arena.context().movement().stepTo(unit, {1, 1}));

	CHECK_EQ(unit.position(), (Position{1, 1}));
	CHECK(!arena.world.map().isOccupied({0, 0}));
	CHECK(arena.world.map().occupant({1, 1}) == UnitId{1});
	CHECK(arena.log == (Log{"[1] moved 1 1,1"}));
}

TEST(MovementService_refuses_illegal_steps_without_changes)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	const Unit& tower = arena.spawn({.id = 2, .position = {5, 5}, .speed = std::nullopt});
	Unit& corpse = arena.spawn({.id = 3, .position = {1, 0}});
	kill(corpse);
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!arena.context().movement().stepTo(unit, {2, 2}));
	CHECK(!arena.context().movement().stepTo(unit, {-1, 0}));
	CHECK(!arena.context().movement().stepTo(unit, {1, 0}));
	CHECK(!arena.context().movement().stepTo(tower, {5, 6}));

	CHECK_EQ(unit.position(), (Position{0, 0}));
	CHECK_EQ(tower.position(), (Position{5, 5}));
	CHECK_EQ(arena.world.activity().changes(), changes);
	CHECK(arena.log.empty());
}

TEST(MovementService_unit_without_cell_occupancy_passes_over_others)
{
	SimArena arena(10, 10);
	arena.spawn({.id = 1, .position = {1, 0}});
	const Unit& raven = arena.spawn({.id = 2, .position = {0, 0}, .occupiesCell = false});

	CHECK(arena.context().movement().stepTo(raven, {1, 0}));
	CHECK(arena.world.map().occupant({1, 0}) == UnitId{1});
}

TEST(MovementService_first_march_starts_at_once_and_next_ones_wait_in_queue)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	arena.march(unit, {3, 0});
	arena.march(unit, {3, 3});

	CHECK(arena.log == (Log{"[1] march started 1 0,0->3,0"}));
	CHECK_EQ(unit.get<sw::core::Movement>().marches.size(), size_t{2});
}

TEST(MarchStep_walks_straight_and_ends_march_on_arrival)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn({.id = 2, .position = {9, 9}, .speed = std::nullopt});
	arena.march(unit, {3, 0});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(arena.simulation.runTick() == std::nullopt);

	CHECK(
		arena.log
		== (Log{
			"[1] march started 1 0,0->3,0",
			"[2] moved 1 1,0",
			"[3] moved 1 2,0",
			"[4] moved 1 3,0",
			"[4] march ended 1 3,0"}));
	CHECK(unit.get<sw::core::Movement>().marches.empty());
}

TEST(MarchStep_goes_around_an_occupied_cell)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn({.id = 2, .position = {1, 0}, .speed = std::nullopt});
	arena.march(unit, {2, 0});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK_EQ(unit.position(), (Position{1, 1}));
}

TEST(MarchStep_waits_when_no_cell_gets_closer)
{
	SimArena arena(3, 1);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn({.id = 2, .position = {1, 0}, .speed = std::nullopt});
	arena.march(unit, {2, 0});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!arena.context().movement().stepTo(unit, {1, 0}));
	sw::core::MarchStep step;
	CHECK(!step.tryExecute(unit, arena.context()));

	CHECK_EQ(unit.position(), (Position{0, 0}));
	CHECK_EQ(arena.world.activity().changes(), changes);
	CHECK_EQ(unit.get<sw::core::Movement>().marches.size(), size_t{1});
}

TEST(MarchStep_speed_two_moves_two_cells_per_tick)
{
	SimArena arena(10, 10);
	const Unit& raven = arena.spawn({.id = 1, .position = {0, 0}, .speed = 2, .occupiesCell = false});
	arena.spawn({.id = 2, .position = {9, 9}, .speed = std::nullopt});
	arena.march(raven, {5, 0});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(arena.log == (Log{"[1] march started 1 0,0->5,0", "[2] moved 1 1,0", "[2] moved 1 2,0"}));
}

TEST(MarchStep_next_march_starts_in_the_same_tick_and_moves_from_the_next)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}, .speed = 2});
	arena.spawn({.id = 2, .position = {9, 9}, .speed = std::nullopt});
	arena.march(unit, {1, 0});
	arena.march(unit, {1, 3});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(arena.simulation.runTick() == std::nullopt);

	CHECK(
		arena.log
		== (Log{
			"[1] march started 1 0,0->1,0",
			"[2] moved 1 1,0",
			"[2] march ended 1 1,0",
			"[2] march started 1 1,0->1,3",
			"[3] moved 1 1,1",
			"[3] moved 1 1,2"}));
}
