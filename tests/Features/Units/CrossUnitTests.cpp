#include "Features/FeatureTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Features/Units/Healer.hpp>
#include <Features/Units/Mine.hpp>
#include <Features/Units/Raven.hpp>
#include <Features/Units/Tower.hpp>
#include <string>
#include <utility>
#include <vector>

using sw::core::Position;
using sw::core::SpawnRequest;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::features;
using namespace sw::test;

// Взаимодействие юнитов из «Планов» между собой: каждая пара проверена на настоящих юнитах.
namespace
{
	using Lines = std::vector<std::string>;

	Unit& place(FeatureArena& arena, SpawnRequest request)
	{
		return arena.world.place(std::move(request.unit).value());
	}

	Unit& raven(FeatureArena& arena, uint32_t id, Position position)
	{
		return place(arena, makeRaven({UnitId{id}, position, 10, 2}));
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

	bool towerShootsRavenAt(int64_t distance)
	{
		FeatureArena arena(10, 10);
		Unit& tower = place(arena, makeTower({UnitId{1}, {0, 0}, 10, 3}));
		raven(arena, 2, {distance, 0});
		return act(tower, arena.context()) && arena.attacks == (Lines{"1->2 damage=3 hp=7"});
	}
}

TEST(Tower_shoots_a_raven_from_one_to_four)
{
	CHECK(towerShootsRavenAt(1));
	CHECK(towerShootsRavenAt(4));
	CHECK(!towerShootsRavenAt(5));
}

TEST(Raven_on_a_mine_arms_it_and_the_explosion_hits_the_raven)
{
	FeatureArena arena(10, 10);
	place(arena, makeMine({UnitId{1}, {5, 5}, 4}));
	const Unit& onMine = raven(arena, 2, {5, 5});
	arena.dummy(3, {0, 9});

	CHECK(!arena.simulation.runTick().has_value());
	CHECK(arena.attacks.empty());
	CHECK(!arena.simulation.runTick().has_value());
	CHECK(arena.attacks == (Lines{"1->2 damage=4 hp=6"}));
	CHECK_EQ(onMine.get<sw::core::Health>().current, int64_t{6});
	CHECK(arena.world.units().find(UnitId{1}) == nullptr);
}

TEST(Healer_heals_a_raven)
{
	FeatureArena arena(10, 10);
	Lines heals;
	arena.bus.subscribe<sw::core::events::UnitHealed>(
		[&heals](const sw::core::events::UnitHealed& e)
		{ heals.push_back(std::to_string(e.targetId.value) + " hp=" + std::to_string(e.targetHp)); });
	Unit& healer = place(arena, makeHealer({UnitId{1}, {5, 5}, 10, 3}));
	Unit& wounded = raven(arena, 2, {5, 5});
	wounded.get<sw::core::Health>().current = 4;

	CHECK(act(healer, arena.context()));
	CHECK(heals == (Lines{"2 hp=7"}));
}
