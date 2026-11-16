#include "Features/FeatureTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Interaction/TargetSpec.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/CommonTags.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Common/TargetSpecs.hpp>
#include <Features/Units/Hunter.hpp>
#include <Features/Units/Raven.hpp>
#include <Features/Units/Swordsman.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using sw::core::Diagnostic;
using sw::core::DistanceRange;
using sw::core::Position;
using sw::core::SpawnRequest;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::features;
using namespace sw::test;

namespace
{
	using Lines = std::vector<std::string>;

	Unit& place(FeatureArena& arena, SpawnRequest request)
	{
		return arena.world.place(std::move(request.unit).value());
	}

	Unit& placeRaven(FeatureArena& arena, uint32_t id, Position position)
	{
		return place(arena, makeRaven({UnitId{id}, position, 5, 2}));
	}

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

TEST(Raven_is_assembled_from_its_spec)
{
	SpawnRequest request = makeRaven({UnitId{1}, {2, 3}, 5, 2});
	CHECK(request.unit.ok());
	CHECK(!request.occupiesCell);

	Unit raven = std::move(request.unit).value();
	CHECK_EQ(raven.type(), std::string_view("Raven"));
	CHECK(!raven.occupiesCell());
	CHECK(raven.tags().has(tags::Flying));
	CHECK_EQ(raven.get<sw::core::Health>().current, int64_t{5});
	CHECK_EQ(raven.get<Agility>().value, int64_t{2});
	CHECK_EQ(raven.get<sw::core::Movement>().speed, int64_t{2});
	CHECK(raven.has<Flying>());
	CHECK_EQ(raven.behaviour().size(), size_t{2});
}

TEST(Raven_reports_all_invalid_stats_at_once)
{
	const SpawnRequest request = makeRaven({UnitId{4}, {1, 1}, 0, 0});
	Lines errors;
	for (const Diagnostic& error : request.unit.errors())
	{
		errors.push_back("[" + error.code + "] " + error.message);
	}
	CHECK(
		errors
		== (Lines{
			"[unit.invalid_stat] hp must be positive, got 0", "[unit.invalid_stat] agility must be positive, got 0"}));
	CHECK(!request.occupiesCell);
}

TEST(Flying_forbids_melee_and_shortens_ranged_attacks_only)
{
	const Flying flying;
	CHECK(!flying.adjust(attack({tags::Melee, tags::Physical}, {0, 1}), {0, 1}).has_value());
	CHECK(flying.adjust(attack({tags::Ranged, tags::Physical}, {2, 4}), {2, 4}) == (DistanceRange{1, 3}));
	CHECK(flying.adjust(attack({tags::Area}, {0, 3}), {0, 3}) == (DistanceRange{0, 3}));
	CHECK(flying.adjust(heal({tags::Melee}, {0, 2}), {0, 2}) == (DistanceRange{0, 2}));
	CHECK(flying.adjust(buff({tags::Ranged}, {2, 4}), {2, 4}) == (DistanceRange{2, 4}));
	CHECK(flying.adjust(debuff({tags::Melee}, {0, 1}), {0, 1}) == std::nullopt);
}

TEST(Swordsman_does_not_strike_a_raven)
{
	FeatureArena arena(10, 10);
	Unit& swordsman = place(arena, makeSwordsman({UnitId{1}, {5, 5}, 10, 3}));
	placeRaven(arena, 2, {5, 6});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(swordsman, arena.context()));
	CHECK(arena.attacks.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Hunter_shoots_a_raven_from_one_to_range_minus_one)
{
	FeatureArena adjacent(10, 10);
	Unit& nearHunter = place(adjacent, makeHunter({UnitId{1}, {0, 0}, 10, 4, 1, 4}));
	placeRaven(adjacent, 2, {1, 1});
	CHECK(act(nearHunter, adjacent.context()));
	CHECK(adjacent.attacks == (Lines{"1->2 damage=4 hp=1"}));

	FeatureArena farthest(10, 10);
	Unit& farHunter = place(farthest, makeHunter({UnitId{1}, {0, 0}, 10, 4, 1, 4}));
	placeRaven(farthest, 2, {3, 0});
	CHECK(act(farHunter, farthest.context()));
	CHECK(farthest.attacks == (Lines{"1->2 damage=4 hp=1"}));
}

TEST(Hunter_does_not_reach_a_raven_at_its_range)
{
	FeatureArena arena(10, 10);
	Unit& hunter = place(arena, makeHunter({UnitId{1}, {0, 0}, 10, 4, 1, 4}));
	placeRaven(arena, 2, {4, 4});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(hunter, arena.context()));
	CHECK(arena.attacks.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Raven_claws_with_agility_on_its_own_cell_and_adjacent)
{
	FeatureArena own(10, 10);
	Unit& ravenOnCell = placeRaven(own, 1, {5, 5});
	own.dummy(2, {5, 5});
	CHECK(act(ravenOnCell, own.context()));
	CHECK(own.attacks == (Lines{"1->2 damage=2 hp=8"}));

	FeatureArena near(10, 10);
	Unit& ravenNear = placeRaven(near, 1, {5, 5});
	near.dummy(2, {4, 6});
	CHECK(act(ravenNear, near.context()));
	CHECK(near.attacks == (Lines{"1->2 damage=2 hp=8"}));
}

TEST(Raven_does_not_claw_a_raven)
{
	FeatureArena arena(10, 10);
	Unit& raven = placeRaven(arena, 1, {5, 5});
	placeRaven(arena, 2, {5, 5});
	placeRaven(arena, 3, {6, 5});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(raven, arena.context()));
	CHECK(arena.attacks.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Raven_flies_two_cells_per_tick_over_occupied_cells)
{
	FeatureArena arena(10, 1);
	Unit& raven = placeRaven(arena, 1, {0, 0});
	arena.spawn({.id = 2, .position = {1, 0}, .speed = std::nullopt, .hp = std::nullopt});
	arena.spawn({.id = 3, .position = {2, 0}, .speed = std::nullopt, .hp = std::nullopt});
	arena.march(raven, {3, 0});

	CHECK(act(raven, arena.context()));
	CHECK_EQ(raven.position(), (Position{2, 0}));
	CHECK(act(raven, arena.context()));
	CHECK_EQ(raven.position(), (Position{3, 0}));
	CHECK(raven.get<sw::core::Movement>().marches.empty());
}
