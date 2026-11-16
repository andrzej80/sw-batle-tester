#include "App/AppTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <App/Application.hpp>
#include <string>

using namespace sw::test;

TEST(SpawnMine_spawns_a_mine_on_an_occupied_cell)
{
	const AppRun run = runApplication(
		"mine_spawn",
		"CREATE_MAP 10 10\n"
		"SPAWN_SWORDSMAN 1 9 9 10 1\n"
		"SPAWN_MINE 2 9 9 4\n");

	CHECK_EQ(run.code, sw::app::exit_code::Success);
	CHECK_EQ(run.err, std::string());
	CHECK(run.out.find("[1] UNIT_SPAWNED unitId=2 unitType=Mine x=9 y=9") != std::string::npos);
	CHECK(run.out.find("[3] MINE_EXPLODED unitId=2 \n") != std::string::npos);
	CHECK(run.out.find("[3] UNIT_ATTACKED attackerUnitId=2 targetUnitId=1 damage=4 targetHp=6") != std::string::npos);
}

TEST(SpawnMine_reports_parse_stat_and_march_errors)
{
	const AppRun run = runApplication(
		"mine_errors",
		"CREATE_MAP 10 10\n"
		"SPAWN_MINE 1 0 0\n"
		"SPAWN_MINE 2 0 0 4 7\n"
		"SPAWN_MINE 3 1 1 0\n"
		"SPAWN_MINE 4 5 5 4\n"
		"MARCH 4 0 5\n");

	CHECK_EQ(run.code, sw::app::exit_code::InvalidScenario);
	CHECK_EQ(
		run.err,
		std::string(
			"error: scenario.txt:2: SPAWN_MINE [parse.missing_argument]: missing argument 'power'\n"
			"error: scenario.txt:3: SPAWN_MINE [parse.extra_argument]: unexpected extra argument '7'\n"
			"error: scenario.txt:4: SPAWN_MINE [unit.invalid_stat]: power must be positive, got 0\n"
			"error: scenario.txt:6: MARCH [march.unit_cannot_move]: unit 4 (Mine) cannot move (unit 4 spawned at line "
			"5)\n"
			"4 errors, simulation not started.\n"));
}
