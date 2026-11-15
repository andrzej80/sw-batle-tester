#include "Features/FeatureTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Units/Hunter.hpp>
#include <Features/Units/Swordsman.hpp>
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

	Lines errorsOf(const SpawnRequest& request)
	{
		Lines lines;
		for (const Diagnostic& error : request.unit.errors())
		{
			lines.push_back("[" + error.code + "] " + error.message);
		}
		return lines;
	}

	Unit& place(FeatureArena& arena, SpawnRequest request)
	{
		return arena.world.place(std::move(request.unit).value());
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

TEST(Swordsman_is_assembled_from_its_spec)
{
	SpawnRequest request = makeSwordsman({UnitId{1}, {2, 3}, 5, 2});
	CHECK(request.unit.ok());
	CHECK(request.occupiesCell);

	const Unit swordsman = std::move(request.unit).value();
	CHECK_EQ(swordsman.type(), std::string_view("Swordsman"));
	CHECK_EQ(swordsman.position(), (Position{2, 3}));
	CHECK_EQ(swordsman.get<sw::core::Health>().current, int64_t{5});
	CHECK_EQ(swordsman.get<Strength>().value, int64_t{2});
	CHECK_EQ(swordsman.get<sw::core::Movement>().speed, int64_t{1});
}

// Приёмка п. 9.3: все ошибки статов — одним списком
TEST(Swordsman_reports_all_invalid_stats_at_once)
{
	const SpawnRequest request = makeSwordsman({UnitId{4}, {1, 1}, 0, 0});
	CHECK(
		errorsOf(request)
		== (Lines{
			"[unit.invalid_stat] hp must be positive, got 0", "[unit.invalid_stat] strength must be positive, got 0"}));
	CHECK_EQ(request.id, UnitId{4});
	CHECK_EQ(request.position, (Position{1, 1}));
	CHECK(request.occupiesCell);
}

TEST(Hunter_is_assembled_from_its_spec)
{
	SpawnRequest request = makeHunter({UnitId{2}, {9, 0}, 10, 5, 1, 4});
	CHECK(request.unit.ok());

	Unit hunter = std::move(request.unit).value();	// behaviour() — только у изменяемого юнита
	CHECK_EQ(hunter.type(), std::string_view("Hunter"));
	CHECK_EQ(hunter.get<Agility>().value, int64_t{5});
	CHECK_EQ(hunter.get<Strength>().value, int64_t{1});
	CHECK_EQ(hunter.get<Range>().value, int64_t{4});
	CHECK_EQ(hunter.behaviour().size(), size_t{3});
}

TEST(Hunter_reports_all_invalid_stats_and_range_at_once)
{
	CHECK(
		errorsOf(makeHunter({UnitId{2}, {0, 0}, 0, 0, 0, 1}))
		== (Lines{
			"[unit.invalid_stat] hp must be positive, got 0",
			"[unit.invalid_stat] agility must be positive, got 0",
			"[unit.invalid_stat] strength must be positive, got 0",
			"[hunter.invalid_range] range must be >= 2, got 1"}));
	CHECK(
		errorsOf(makeHunter({UnitId{2}, {0, 0}, 10, 5, 1, 0}))
		== (Lines{"[hunter.invalid_range] range must be >= 2, got 0"}));
	CHECK(makeHunter({UnitId{2}, {0, 0}, 10, 5, 1, 2}).unit.ok());
}

// Приёмка п. 9.3: Охотник с соседом бьёт Strength
TEST(Hunter_with_a_neighbour_strikes_from_the_shadow)
{
	FeatureArena arena(10, 10);
	Unit& hunter = place(arena, makeHunter({UnitId{1}, {5, 5}, 10, 5, 1, 4}));
	arena.dummy(2, {6, 5});	 // сосед
	arena.dummy(3, {8, 5});	 // на дистанции выстрела, но выстрелить нельзя

	CHECK(act(hunter, arena.context()));
	CHECK(arena.attacks == (Lines{"1->2 damage=1 hp=9"}));
}

// Приёмка п. 9.3: без соседей и с целью на дистанции 2..range стреляет Agility
TEST(Hunter_without_neighbours_shoots_with_agility)
{
	FeatureArena arena(10, 10);
	Unit& hunter = place(arena, makeHunter({UnitId{1}, {5, 5}, 10, 5, 1, 4}));
	arena.dummy(2, {9, 9});	 // дистанция 4

	CHECK(act(hunter, arena.context()));
	CHECK(arena.attacks == (Lines{"1->2 damage=5 hp=5"}));
}

// Приёмка п. 9.3: цель на дистанции 1 и сосед-летающий не мешают выстрелу
TEST(Hunter_shoots_past_a_flying_neighbour)
{
	FeatureArena arena(10, 10);
	Unit& hunter = place(arena, makeHunter({UnitId{1}, {5, 5}, 10, 5, 1, 4}));
	arena.dummy(2, {5, 6}, /*occupiesCell*/ false);	 // «Ворон» рядом
	arena.dummy(3, {5, 8});

	CHECK(act(hunter, arena.context()));
	CHECK(arena.attacks == (Lines{"1->3 damage=5 hp=5"}));
}

TEST(Hunter_without_targets_follows_its_march)
{
	FeatureArena arena(10, 10);
	Unit& hunter = place(arena, makeHunter({UnitId{1}, {5, 5}, 10, 5, 1, 2}));
	arena.dummy(2, {9, 9});	 // дистанция 4 > range 2
	arena.march(hunter, {0, 5});

	CHECK(act(hunter, arena.context()));
	CHECK(arena.attacks.empty());
	CHECK_EQ(hunter.position(), (Position{4, 5}));
}

// Приёмка п. 9.3: Мечник без соседей и без марша бездействует
TEST(Swordsman_without_neighbours_and_march_stays_idle)
{
	FeatureArena arena(10, 10);
	Unit& swordsman = place(arena, makeSwordsman({UnitId{1}, {5, 5}, 5, 2}));
	arena.dummy(2, {7, 7});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(swordsman, arena.context()));
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Swordsman_strikes_a_neighbour_instead_of_marching)
{
	FeatureArena arena(10, 10);
	Unit& swordsman = place(arena, makeSwordsman({UnitId{1}, {5, 5}, 5, 2}));
	arena.dummy(2, {4, 4});
	arena.march(swordsman, {9, 5});

	CHECK(act(swordsman, arena.context()));
	CHECK(arena.attacks == (Lines{"1->2 damage=2 hp=8"}));
	CHECK_EQ(swordsman.position(), (Position{5, 5}));
	CHECK_EQ(swordsman.get<sw::core::Movement>().marches.size(), size_t{1});  // марш не отменён
}
