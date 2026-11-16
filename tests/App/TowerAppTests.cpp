#include "App/AppTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <App/Application.hpp>
#include <string>

using namespace sw::test;

TEST(SpawnTower_spawns_a_tower)
{
	const AppRun run = runApplication("tower_spawn", "CREATE_MAP 10 10\nSPAWN_TOWER 1 3 4 10 4\n");

	CHECK_EQ(run.code, sw::app::exit_code::Success);
	CHECK_EQ(run.err, std::string());
	CHECK(run.out.find("[1] UNIT_SPAWNED unitId=1 unitType=Tower x=3 y=4") != std::string::npos);
}

TEST(SpawnTower_reports_parse_stat_and_march_errors)
{
	const AppRun run = runApplication(
		"tower_errors",
		"CREATE_MAP 10 10\n"
		"SPAWN_TOWER 1 0 0 10\n"
		"SPAWN_TOWER 2 0 0 10 4 7\n"
		"SPAWN_TOWER 3 1 1 0 0\n"
		"SPAWN_TOWER 4 5 5 10 4\n"
		"MARCH 4 0 5\n");

	CHECK_EQ(run.code, sw::app::exit_code::InvalidScenario);
	CHECK_EQ(
		run.err,
		std::string(
			"error: scenario.txt:2: SPAWN_TOWER [parse.missing_argument]: missing argument 'power'\n"
			"error: scenario.txt:3: SPAWN_TOWER [parse.extra_argument]: unexpected extra argument '7'\n"
			"error: scenario.txt:4: SPAWN_TOWER [unit.invalid_stat]: hp must be positive, got 0\n"
			"error: scenario.txt:4: SPAWN_TOWER [unit.invalid_stat]: power must be positive, got 0\n"
			"error: scenario.txt:6: MARCH [march.unit_cannot_move]: unit 4 (Tower) cannot move (unit 4 spawned at line "
			"5)\n"
			"5 errors, simulation not started.\n"));
	CHECK_EQ(run.out, std::string());
}
