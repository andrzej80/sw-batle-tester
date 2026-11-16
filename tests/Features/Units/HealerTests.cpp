#include "Features/FeatureTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Units/Healer.hpp>
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

	struct HealerArena : FeatureArena
	{
		HealerArena() :
				FeatureArena(10, 10)
		{
			bus.subscribe<sw::core::events::UnitHealed>(
				[this](const sw::core::events::UnitHealed& e)
				{
					heals.push_back(
						std::to_string(e.healerId.value) + "->" + std::to_string(e.targetId.value)
						+ " amount=" + std::to_string(e.amount) + " hp=" + std::to_string(e.targetHp));
				});
		}

		Unit& healer(uint32_t id, Position position, int64_t spirit = 3)
		{
			SpawnRequest request = makeHealer({UnitId{id}, position, 10, spirit});
			return world.place(std::move(request.unit).value());
		}

		Unit& wounded(uint32_t id, Position position, int64_t hp, bool occupiesCell = true)
		{
			Unit& unit = dummy(id, position, occupiesCell);
			unit.get<sw::core::Health>().current = hp;
			return unit;
		}

		std::vector<std::string> heals;
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

TEST(Healer_is_assembled_from_its_spec)
{
	SpawnRequest request = makeHealer({UnitId{1}, {2, 3}, 10, 4});
	CHECK(request.unit.ok());
	CHECK(request.occupiesCell);

	Unit healer = std::move(request.unit).value();
	CHECK_EQ(healer.type(), std::string_view("Healer"));
	CHECK_EQ(healer.get<sw::core::Health>().current, int64_t{10});
	CHECK_EQ(healer.get<Spirit>().value, int64_t{4});
	CHECK_EQ(healer.get<sw::core::Movement>().speed, int64_t{1});
	CHECK_EQ(healer.behaviour().size(), size_t{2});
}

TEST(Healer_reports_all_invalid_stats_at_once)
{
	const SpawnRequest request = makeHealer({UnitId{4}, {1, 1}, 0, 0});
	Lines errors;
	for (const Diagnostic& error : request.unit.errors())
	{
		errors.push_back("[" + error.code + "] " + error.message);
	}
	CHECK(
		errors
		== (Lines{
			"[unit.invalid_stat] hp must be positive, got 0", "[unit.invalid_stat] spirit must be positive, got 0"}));
}

TEST(Healer_heals_a_wounded_unit_at_distance_one_and_two)
{
	HealerArena near;
	Unit& nearHealer = near.healer(1, {5, 5});
	near.wounded(2, {6, 6}, 5);
	CHECK(act(nearHealer, near.context()));
	CHECK(near.heals == (Lines{"1->2 amount=3 hp=8"}));

	HealerArena far;
	Unit& farHealer = far.healer(1, {5, 5});
	far.wounded(2, {5, 7}, 5);
	CHECK(act(farHealer, far.context()));
	CHECK(far.heals == (Lines{"1->2 amount=3 hp=8"}));
}

TEST(Healer_does_not_reach_distance_three)
{
	HealerArena arena;
	Unit& healer = arena.healer(1, {5, 5});
	arena.wounded(2, {8, 5}, 5);
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(healer, arena.context()));
	CHECK(arena.heals.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Healer_does_not_heal_units_with_full_hp)
{
	HealerArena arena;
	Unit& healer = arena.healer(1, {5, 5});
	arena.dummy(2, {6, 5});
	arena.dummy(3, {5, 5}, /*occupiesCell*/ false);
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(healer, arena.context()));
	CHECK(arena.heals.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Healer_does_not_heal_itself)
{
	HealerArena arena;
	Unit& healer = arena.healer(1, {5, 5});
	healer.get<sw::core::Health>().current = 2;
	arena.dummy(2, {9, 9});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(healer, arena.context()));
	CHECK(arena.heals.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(Healer_heals_a_unit_on_its_own_cell)
{
	HealerArena arena;
	Unit& healer = arena.healer(1, {5, 5});
	arena.wounded(2, {5, 5}, 4, /*occupiesCell*/ false);

	CHECK(act(healer, arena.context()));
	CHECK(arena.heals == (Lines{"1->2 amount=3 hp=7"}));
}

TEST(Healer_does_not_raise_hp_above_max_and_reports_the_actual_amount)
{
	HealerArena arena;
	Unit& healer = arena.healer(1, {5, 5}, 5);
	const Unit& target = arena.wounded(2, {6, 5}, 9);

	CHECK(act(healer, arena.context()));
	CHECK(arena.heals == (Lines{"1->2 amount=1 hp=10"}));
	CHECK_EQ(target.get<sw::core::Health>().current, int64_t{10});
}

TEST(Healer_without_targets_follows_its_march)
{
	HealerArena arena;
	Unit& healer = arena.healer(1, {5, 5});
	arena.dummy(2, {9, 9});
	arena.march(healer, {0, 5});

	CHECK(act(healer, arena.context()));
	CHECK(arena.heals.empty());
	CHECK_EQ(healer.position(), (Position{4, 5}));
}

TEST(Two_healers_with_full_hp_end_the_simulation_on_tick_two)
{
	HealerArena arena;
	arena.healer(1, {5, 5});
	arena.healer(2, {6, 5});

	CHECK(arena.simulation.run() == sw::core::StopReason::NothingHappened);
	CHECK_EQ(arena.world.tick(), sw::core::Tick{2});
	CHECK(arena.heals.empty());
}
