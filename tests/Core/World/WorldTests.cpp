#include "Core/World/WorldTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/World/World.hpp>
#include <stdexcept>
#include <vector>

using sw::core::ActivityTracker;
using sw::core::DistanceRange;
using sw::core::Position;
using sw::core::Unit;
using sw::core::UnitId;
using sw::core::World;
using namespace sw::test;

namespace
{
	std::vector<uint32_t> ids(const std::vector<const Unit*>& units)
	{
		std::vector<uint32_t> result;
		for (const Unit* unit : units)
		{
			result.push_back(unit->id().value);
		}
		return result;
	}
}

TEST(World_place_respects_cell_occupancy)
{
	World world(10, 10);
	world.place(makeUnit({.id = 1, .position = {3, 3}}));

	CHECK_THROWS(world.place(makeUnit({.id = 2, .position = {3, 3}})), std::logic_error);
	world.place(makeUnit({.id = 3, .position = {3, 3}, .occupiesCell = false}));

	CHECK(world.map().occupant({3, 3}) == UnitId{1});
	CHECK_EQ(world.units().size(), size_t{2});
}

TEST(World_place_rejects_duplicate_id_and_cells_outside_the_map)
{
	World world(10, 10);
	world.place(makeUnit({.id = 1, .position = {0, 0}}));
	CHECK_THROWS(world.place(makeUnit({.id = 1, .position = {5, 5}})), std::logic_error);
	CHECK_THROWS(world.place(makeUnit({.id = 2, .position = {10, 0}})), std::logic_error);
	CHECK(!world.map().isOccupied({5, 5}));
}

TEST(World_keeps_creation_order)
{
	World world(10, 10);
	world.place(makeUnit({.id = 5, .position = {0, 0}}));
	world.place(makeUnit({.id = 2, .position = {1, 0}}));
	world.place(makeUnit({.id = 9, .position = {2, 0}}));

	const std::vector<UnitId> expected{UnitId{5}, UnitId{2}, UnitId{9}};
	CHECK(world.units().orderedIds() == expected);
}

TEST(World_dead_unit_keeps_its_cell_until_cleanup)
{
	World world(10, 10);
	Unit& victim = world.place(makeUnit({.id = 1, .position = {4, 4}}));
	world.place(makeUnit({.id = 2, .position = {5, 4}}));
	kill(victim);

	const Unit walker = makeUnit({.id = 3, .position = {6, 4}});
	CHECK(world.map().isOccupied({4, 4}));
	CHECK(!world.query().isFreeFor(walker, {4, 4}));

	CHECK_EQ(world.removePending(), size_t{1});
	CHECK(!world.map().isOccupied({4, 4}));
	CHECK(world.query().isFreeFor(walker, {4, 4}));
	CHECK(world.units().find(UnitId{1}) == nullptr);
	CHECK_EQ(world.units().orderedIds().size(), size_t{1});
}

TEST(World_removes_units_marked_for_removal_without_health)
{
	World world(10, 10);
	Unit& mine = world.place(makeUnit({.id = 1, .position = {4, 4}, .occupiesCell = false, .hp = std::nullopt}));
	world.place(makeUnit({.id = 2, .position = {4, 4}}));

	CHECK_EQ(world.removePending(), size_t{0});
	mine.requestRemoval();
	CHECK_EQ(world.removePending(), size_t{1});
	CHECK(world.map().occupant({4, 4}) == UnitId{2});
}

TEST(SpatialQuery_finds_only_alive_units_in_range)
{
	World world(10, 10);
	world.place(makeUnit({.id = 1, .position = {0, 0}}));
	Unit& dead = world.place(makeUnit({.id = 2, .position = {2, 0}}));
	world.place(makeUnit({.id = 3, .position = {3, 0}}));
	world.place(makeUnit({.id = 4, .position = {6, 0}}));
	Unit& removed = world.place(makeUnit({.id = 5, .position = {2, 1}, .hp = std::nullopt}));
	kill(dead);
	removed.requestRemoval();

	const std::vector<uint32_t> inRange = ids(world.query().unitsInRange({0, 0}, DistanceRange{2, 5}));
	CHECK(inRange == (std::vector<uint32_t>{3}));
	CHECK(ids(world.query().unitsInRange({0, 0}, DistanceRange{0, 0})) == (std::vector<uint32_t>{1}));
	CHECK(ids(world.query().unitsInRange({0, 0}, DistanceRange{3, 2})).empty());
	CHECK_EQ(world.query().aliveCount(), size_t{3});
}

TEST(SpatialQuery_neighbours_are_alive_units_at_distance_one)
{
	World world(10, 10);
	world.place(makeUnit({.id = 1, .position = {5, 5}}));
	world.place(makeUnit({.id = 2, .position = {6, 6}}));
	world.place(makeUnit({.id = 3, .position = {5, 5}, .occupiesCell = false}));
	world.place(makeUnit({.id = 4, .position = {7, 5}}));

	CHECK(ids(world.query().neighbours({5, 5})) == (std::vector<uint32_t>{2}));
}

TEST(SpatialQuery_free_cell_depends_on_occupancy_of_the_mover)
{
	World world(10, 10);
	world.place(makeUnit({.id = 1, .position = {5, 5}}));
	const Unit walker = makeUnit({.id = 2, .position = {4, 4}});
	const Unit flyer = makeUnit({.id = 3, .position = {4, 4}, .occupiesCell = false});

	CHECK(!world.query().isFreeFor(walker, {5, 5}));
	CHECK(world.query().isFreeFor(flyer, {5, 5}));
	CHECK(world.query().isFreeFor(walker, {4, 5}));
	CHECK(!world.query().isFreeFor(flyer, {10, 5}));
}

TEST(ActivityTracker_empty_tick_and_change_counter)
{
	ActivityTracker tracker;
	tracker.beginTick();
	CHECK(tracker.tickWasEmpty());

	tracker.markChange();
	CHECK(tracker.tickWasEmpty());
	CHECK_EQ(tracker.changes(), uint64_t{1});

	tracker.markAction();
	CHECK(!tracker.tickWasEmpty());

	tracker.beginTick();
	CHECK(tracker.tickWasEmpty());
	CHECK_EQ(tracker.changes(), uint64_t{1});

	tracker.markScheduled();
	CHECK(!tracker.tickWasEmpty());
	tracker.beginTick();
	tracker.markRemoval();
	CHECK(!tracker.tickWasEmpty());
}
