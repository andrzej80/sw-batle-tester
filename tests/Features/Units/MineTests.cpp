#include "Features/FeatureTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Units/Hunter.hpp>
#include <Features/Units/Mine.hpp>
#include <Features/Units/Swordsman.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using sw::core::Diagnostic;
using sw::core::Position;
using sw::core::SpawnRequest;
using sw::core::StopReason;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::features;
using namespace sw::test;

namespace
{
	using Lines = std::vector<std::string>;

	struct MineArena : FeatureArena
	{
		MineArena() :
				FeatureArena(10, 10)
		{
			namespace events = sw::core::events;
			bus.subscribe<MineExploded>([this](const MineExploded& e)
										{ timeline.push_back(at() + "exploded " + std::to_string(e.unitId.value)); });
			bus.subscribe<events::UnitAttacked>(
				[this](const events::UnitAttacked& e)
				{
					timeline.push_back(
						at() + std::to_string(e.attackerId.value) + "->" + std::to_string(e.targetId.value)
						+ " damage=" + std::to_string(e.damage) + " hp=" + std::to_string(e.targetHp));
				});
			bus.subscribe<events::UnitDied>([this](const events::UnitDied& e)
											{ timeline.push_back(at() + "died " + std::to_string(e.unitId.value)); });
		}

		Unit& mine(uint32_t id, Position position, int64_t power = 4)
		{
			return place(makeMine({UnitId{id}, position, power}));
		}

		Unit& place(SpawnRequest request)
		{
			return world.place(std::move(request.unit).value());
		}

		bool tick()
		{
			return !simulation.runTick().has_value();
		}

		std::string at() const
		{
			return "[" + std::to_string(world.tick()) + "] ";
		}

		Lines timeline;
	};

	bool act(Unit& unit, sw::core::TurnContext& context)
	{
		for (const auto& action : unit.behaviour())
		{
			if (action->tryExecute(unit, context))
			{
				return true;
			}
		}
		return false;
	}
}

TEST(Mine_is_assembled_from_its_spec)
{
	SpawnRequest request = makeMine({UnitId{1}, {2, 3}, 4});
	CHECK(request.unit.ok());
	CHECK(!request.occupiesCell);

	Unit mine = std::move(request.unit).value();
	CHECK_EQ(mine.type(), std::string_view("Mine"));
	CHECK(!mine.occupiesCell());
	CHECK_EQ(mine.get<Power>().value, int64_t{4});
	CHECK(!mine.has<sw::core::Health>());
	CHECK(!mine.has<sw::core::Movement>());
	CHECK_EQ(mine.behaviour().size(), size_t{1});
}

TEST(Mine_reports_invalid_power)
{
	const SpawnRequest request = makeMine({UnitId{4}, {1, 1}, 0});
	Lines errors;
	for (const Diagnostic& error : request.unit.errors())
	{
		errors.push_back("[" + error.code + "] " + error.message);
	}
	CHECK(errors == (Lines{"[unit.invalid_stat] power must be positive, got 0"}));
	CHECK(!request.occupiesCell);
}

TEST(Mine_cannot_be_attacked_and_does_not_block_its_cell)
{
	MineArena melee;
	melee.mine(1, {5, 5});
	Unit& swordsman = melee.place(makeSwordsman({UnitId{2}, {5, 6}, 10, 3}));
	const uint64_t meleeChanges = melee.world.activity().changes();
	CHECK(!act(swordsman, melee.context()));
	CHECK(melee.attacks.empty());
	CHECK_EQ(melee.world.activity().changes(), meleeChanges);
	CHECK(melee.world.query().isFreeFor(swordsman, {5, 5}));

	MineArena ranged;
	ranged.mine(1, {5, 5});
	Unit& hunter = ranged.place(makeHunter({UnitId{2}, {5, 8}, 10, 4, 1, 4}));
	const uint64_t rangedChanges = ranged.world.activity().changes();
	CHECK(!act(hunter, ranged.context()));
	CHECK(ranged.attacks.empty());
	CHECK_EQ(ranged.world.activity().changes(), rangedChanges);
}

TEST(Mine_is_not_armed_by_another_mine_or_by_a_unit_at_distance_three)
{
	MineArena arena;
	Unit& mine = arena.mine(1, {5, 5});
	arena.mine(2, {5, 6});
	arena.dummy(3, {8, 5});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(mine, arena.context()));
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Mine_is_armed_once_by_a_unit_at_distance_two_even_a_flying_one)
{
	MineArena arena;
	Unit& mine = arena.mine(1, {5, 5});
	arena.dummy(2, {7, 7}, /*occupiesCell*/ false);

	CHECK(act(mine, arena.context()));
	const uint64_t changes = arena.world.activity().changes();
	CHECK(!act(mine, arena.context()));
	CHECK_EQ(arena.world.activity().changes(), changes);
	CHECK(arena.timeline.empty());
}

TEST(Mine_explodes_next_tick_hitting_everyone_within_three_and_disappears)
{
	MineArena arena;
	arena.mine(1, {5, 5});
	arena.dummy(2, {7, 5});
	arena.dummy(3, {5, 5});
	arena.dummy(4, {8, 8});
	arena.dummy(5, {9, 5});
	arena.dummy(6, {5, 4}, /*occupiesCell*/ true, /*hp*/ 3);

	CHECK(arena.tick());
	CHECK(arena.timeline.empty());
	CHECK(arena.tick());
	CHECK(
		arena.timeline
		== (Lines{
			"[3] exploded 1",
			"[3] 1->2 damage=4 hp=6",
			"[3] 1->3 damage=4 hp=6",
			"[3] 1->4 damage=4 hp=6",
			"[3] 1->6 damage=4 hp=0",
			"[3] died 6"}));
	CHECK(arena.world.units().find(UnitId{1}) == nullptr);
	CHECK(arena.world.units().find(UnitId{6}) == nullptr);
	CHECK_EQ(arena.world.units().find(UnitId{5})->get<sw::core::Health>().current, int64_t{10});
}

TEST(Units_killed_by_the_explosion_do_not_act_in_that_tick)
{
	MineArena arena;
	Lines turns;
	arena.mine(1, {5, 5});
	arena.spawn(
		{.id = 2,
		 .position = {6, 5},
		 .hp = 3,
		 .actions = {[&](const Unit&, sw::core::TurnContext& context)
					 {
						 turns.push_back("unit 2 at " + std::to_string(context.tick()));
						 return false;
					 }}});
	arena.dummy(3, {9, 9});
	arena.dummy(4, {0, 9});

	CHECK(arena.tick());
	CHECK(arena.tick());
	CHECK(turns == (Lines{"unit 2 at 2"}));
	CHECK(arena.timeline == (Lines{"[3] exploded 1", "[3] 1->2 damage=4 hp=0", "[3] died 2"}));
}

TEST(Mine_explodes_even_if_the_unit_that_armed_it_has_left)
{
	MineArena arena;
	arena.mine(1, {5, 5});
	const Unit& runner = arena.spawn({.id = 2, .position = {7, 5}, .speed = 2});
	arena.march(runner, {9, 5});
	arena.dummy(3, {0, 9});

	CHECK(arena.tick());
	CHECK_EQ(runner.position(), (Position{9, 5}));
	CHECK(arena.tick());
	CHECK(arena.timeline == (Lines{"[3] exploded 1"}));
	CHECK(arena.world.units().find(UnitId{1}) == nullptr);
}

TEST(Mine_and_a_unit_out_of_its_reach_stop_the_simulation_on_tick_two)
{
	MineArena arena;
	arena.mine(1, {0, 0});
	arena.dummy(2, {9, 9});

	CHECK(arena.simulation.run() == StopReason::NothingHappened);
	CHECK_EQ(arena.world.tick(), sw::core::Tick{2});
	CHECK_EQ(arena.world.units().size(), size_t{2});
}
