#include "Core/Simulation/SimulationTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <Core/World/World.hpp>
#include <Features/Units/Hunter.hpp>
#include <Features/Units/Swordsman.hpp>
#include <string>
#include <vector>

using sw::core::StopReason;
using sw::core::UnitId;
using namespace sw::features;
namespace events = sw::core::events;

namespace
{
	using Lines = std::vector<std::string>;

	std::string xy(sw::core::Position position)
	{
		return "x=" + std::to_string(position.x) + " y=" + std::to_string(position.y);
	}

	void record(sw::core::EventBus& bus, const sw::core::World* const& world, Lines& log)
	{
		const auto at = [&world]()
		{
			return "[" + std::to_string(world ? world->tick() : 1) + "] ";
		};
		bus.subscribe<events::MapCreated>(
			[&, at](const events::MapCreated& e) {
				log.push_back(
					at() + "MAP_CREATED width=" + std::to_string(e.width) + " height=" + std::to_string(e.height));
			});
		bus.subscribe<events::UnitSpawned>(
			[&, at](const events::UnitSpawned& e)
			{
				log.push_back(
					at() + "UNIT_SPAWNED unitId=" + std::to_string(e.unitId.value) + " unitType=" + e.unitType + " "
					+ xy(e.position));
			});
		bus.subscribe<events::MarchStarted>(
			[&, at](const events::MarchStarted& e)
			{
				log.push_back(
					at() + "MARCH_STARTED unitId=" + std::to_string(e.unitId.value) + " " + xy(e.from)
					+ " targetX=" + std::to_string(e.target.x) + " targetY=" + std::to_string(e.target.y));
			});
		bus.subscribe<events::MarchEnded>(
			[&, at](const events::MarchEnded& e)
			{ log.push_back(at() + "MARCH_ENDED unitId=" + std::to_string(e.unitId.value) + " " + xy(e.position)); });
		bus.subscribe<events::UnitMoved>(
			[&, at](const events::UnitMoved& e)
			{ log.push_back(at() + "UNIT_MOVED unitId=" + std::to_string(e.unitId.value) + " " + xy(e.position)); });
		bus.subscribe<events::UnitAttacked>(
			[&, at](const events::UnitAttacked& e)
			{
				log.push_back(
					at() + "UNIT_ATTACKED attackerUnitId=" + std::to_string(e.attackerId.value)
					+ " targetUnitId=" + std::to_string(e.targetId.value) + " damage=" + std::to_string(e.damage)
					+ " targetHp=" + std::to_string(e.targetHp));
			});
		bus.subscribe<events::UnitDied>(
			[&, at](const events::UnitDied& e)
			{ log.push_back(at() + "UNIT_DIED unitId=" + std::to_string(e.unitId.value)); });
	}
}

TEST(ExampleScenario_plays_by_the_rules_with_real_units)
{
	sw::core::EventBus bus;
	sw::core::SetupContext setup(bus);
	const sw::core::World* world = nullptr;
	Lines log;
	record(bus, world, log);

	CHECK(setup.createMap(10, 10).ok());
	world = setup.world();
	CHECK(setup.spawn(makeSwordsman({UnitId{1}, {0, 0}, 5, 2})).ok());
	CHECK(setup.spawn(makeHunter({UnitId{2}, {9, 0}, 10, 5, 1, 4})).ok());
	CHECK(setup.orderMarch(UnitId{1}, {9, 0}).ok());
	CHECK(setup.orderMarch(UnitId{2}, {0, 0}).ok());
	CHECK(setup.spawn(makeSwordsman({UnitId{3}, {0, 9}, 10, 2})).ok());
	CHECK(setup.orderMarch(UnitId{3}, {0, 0}).ok());
	CHECK(setup.finish().ok());

	sw::test::FirstRandom random;
	sw::core::Simulation simulation(*setup.world(), bus, random);
	CHECK(simulation.run() == StopReason::OneOrNoUnitsLeft);

	const Lines expected{
		"[1] MAP_CREATED width=10 height=10",
		"[1] UNIT_SPAWNED unitId=1 unitType=Swordsman x=0 y=0",
		"[1] UNIT_SPAWNED unitId=2 unitType=Hunter x=9 y=0",
		"[1] MARCH_STARTED unitId=1 x=0 y=0 targetX=9 targetY=0",
		"[1] MARCH_STARTED unitId=2 x=9 y=0 targetX=0 targetY=0",
		"[1] UNIT_SPAWNED unitId=3 unitType=Swordsman x=0 y=9",
		"[1] MARCH_STARTED unitId=3 x=0 y=9 targetX=0 targetY=0",
		"[2] UNIT_MOVED unitId=1 x=1 y=0",
		"[2] UNIT_MOVED unitId=2 x=8 y=0",
		"[2] UNIT_MOVED unitId=3 x=0 y=8",
		"[3] UNIT_MOVED unitId=1 x=2 y=0",
		"[3] UNIT_MOVED unitId=2 x=7 y=0",
		"[3] UNIT_MOVED unitId=3 x=0 y=7",
		"[4] UNIT_MOVED unitId=1 x=3 y=0",
		"[4] UNIT_ATTACKED attackerUnitId=2 targetUnitId=1 damage=5 targetHp=0",
		"[4] UNIT_DIED unitId=1",
		"[4] UNIT_MOVED unitId=3 x=0 y=6",
		"[5] UNIT_MOVED unitId=2 x=6 y=0",
		"[5] UNIT_MOVED unitId=3 x=0 y=5",
		"[6] UNIT_MOVED unitId=2 x=5 y=0",
		"[6] UNIT_MOVED unitId=3 x=0 y=4",
		"[7] UNIT_MOVED unitId=2 x=4 y=0",
		"[7] UNIT_MOVED unitId=3 x=0 y=3",
		"[8] UNIT_ATTACKED attackerUnitId=2 targetUnitId=3 damage=5 targetHp=5",
		"[8] UNIT_MOVED unitId=3 x=0 y=2",
		"[9] UNIT_ATTACKED attackerUnitId=2 targetUnitId=3 damage=5 targetHp=0",
		"[9] UNIT_DIED unitId=3",
	};
	CHECK(log == expected);
	if (log != expected)
	{
		for (const std::string& line : log)
		{
			sw::test::reportFailure({__FILE__, __LINE__}, "actual: " + line);
		}
	}
}
