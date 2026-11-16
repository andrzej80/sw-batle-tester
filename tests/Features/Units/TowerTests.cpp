#include "Features/FeatureTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Units/Tower.hpp>
#include <string>
#include <utility>
#include <vector>

using sw::core::Diagnostic;
using sw::core::Position;
using sw::core::SpawnRequest;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::features;
using namespace sw::test;

namespace
{
	using Lines = std::vector<std::string>;

	Unit& placeTower(FeatureArena& arena, Position position, int64_t power = 4)
	{
		SpawnRequest request = makeTower({UnitId{1}, position, 10, power});
		return arena.world.place(std::move(request.unit).value());
	}

	bool act(Unit& unit, sw::core::TurnContext& context)
	{
		return unit.behaviour().front()->tryExecute(unit, context);
	}
}

TEST(Tower_is_assembled_from_its_spec)
{
	SpawnRequest request = makeTower({UnitId{1}, {2, 3}, 10, 4});
	CHECK(request.unit.ok());
	CHECK(request.occupiesCell);

	Unit tower = std::move(request.unit).value();
	CHECK_EQ(tower.type(), std::string_view("Tower"));
	CHECK_EQ(tower.position(), (Position{2, 3}));
	CHECK_EQ(tower.get<sw::core::Health>().current, int64_t{10});
	CHECK_EQ(tower.get<Power>().value, int64_t{4});
	CHECK(!tower.has<sw::core::Movement>());
	CHECK_EQ(tower.behaviour().size(), size_t{1});
}

TEST(Tower_reports_all_invalid_stats_at_once)
{
	const SpawnRequest request = makeTower({UnitId{4}, {1, 1}, 0, 0});
	Lines errors;
	for (const Diagnostic& error : request.unit.errors())
	{
		errors.push_back("[" + error.code + "] " + error.message);
	}
	CHECK(
		errors
		== (Lines{
			"[unit.invalid_stat] hp must be positive, got 0", "[unit.invalid_stat] power must be positive, got 0"}));
	CHECK_EQ(request.id, UnitId{4});
	CHECK(request.occupiesCell);
}

TEST(Tower_shoots_with_power_at_distance_two_and_five)
{
	FeatureArena near(10, 10);
	Unit& nearTower = placeTower(near, {0, 0});
	near.dummy(2, {2, 1});
	CHECK(act(nearTower, near.context()));
	CHECK(near.attacks == (Lines{"1->2 damage=4 hp=6"}));

	FeatureArena far(10, 10);
	Unit& farTower = placeTower(far, {0, 0});
	far.dummy(2, {5, 5});
	CHECK(act(farTower, far.context()));
	CHECK(far.attacks == (Lines{"1->2 damage=4 hp=6"}));
}

TEST(Tower_does_not_shoot_at_distance_one_or_six)
{
	FeatureArena arena(10, 10);
	Unit& tower = placeTower(arena, {0, 0});
	arena.dummy(2, {1, 1});
	arena.dummy(3, {6, 0});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(tower, arena.context()));
	CHECK(arena.attacks.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Tower_shoots_past_an_occupying_neighbour)
{
	FeatureArena arena(10, 10);
	Unit& tower = placeTower(arena, {5, 5});
	arena.dummy(2, {5, 6});
	arena.dummy(3, {5, 8});

	CHECK(act(tower, arena.context()));
	CHECK(arena.attacks == (Lines{"1->3 damage=4 hp=6"}));
}

TEST(Tower_cannot_be_moved)
{
	FeatureArena arena(10, 10);
	const Unit& tower = placeTower(arena, {5, 5});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!arena.context().movement().stepTo(tower, {5, 6}));
	CHECK_EQ(tower.position(), (Position{5, 5}));
	CHECK_EQ(arena.world.activity().changes(), changes);
}
