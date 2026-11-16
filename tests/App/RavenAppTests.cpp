#include "App/AppTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <App/Application.hpp>
#include <string>

using namespace sw::test;

TEST(SpawnRaven_spawns_a_raven_on_an_occupied_cell_and_accepts_a_march)
{
	const AppRun run = runApplication(
		"raven_spawn",
		"CREATE_MAP 10 10\n"
		"SPAWN_SWORDSMAN 1 3 4 10 1\n"
		"SPAWN_RAVEN 2 3 4 5 2\n"
		"MARCH 2 9 9\n");

	CHECK_EQ(run.code, sw::app::exit_code::Success);
	CHECK_EQ(run.err, std::string());
	CHECK(run.out.find("[1] UNIT_SPAWNED unitId=2 unitType=Raven x=3 y=4") != std::string::npos);
	CHECK(run.out.find("[1] MARCH_STARTED unitId=2 x=3 y=4 targetX=9 targetY=9") != std::string::npos);
	CHECK(run.out.find("UNIT_ATTACKED attackerUnitId=2 targetUnitId=1") != std::string::npos);
	CHECK(run.out.find("attackerUnitId=1") == std::string::npos);
}

TEST(SpawnRaven_reports_parse_and_stat_errors)
{
	const AppRun run = runApplication(
		"raven_errors",
		"CREATE_MAP 10 10\n"
		"SPAWN_RAVEN 1 0 0 10\n"
		"SPAWN_RAVEN 2 0 0 10 2 7\n"
		"SPAWN_RAVEN 3 1 1 0 0\n");

	CHECK_EQ(run.code, sw::app::exit_code::InvalidScenario);
	CHECK_EQ(
		run.err,
		std::string("error: scenario.txt:2: SPAWN_RAVEN [parse.missing_argument]: missing argument 'agility'\n"
					"error: scenario.txt:3: SPAWN_RAVEN [parse.extra_argument]: unexpected extra argument '7'\n"
					"error: scenario.txt:4: SPAWN_RAVEN [unit.invalid_stat]: hp must be positive, got 0\n"
					"error: scenario.txt:4: SPAWN_RAVEN [unit.invalid_stat]: agility must be positive, got 0\n"
					"4 errors, simulation not started.\n"));
}
