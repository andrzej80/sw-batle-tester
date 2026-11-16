#include "App/AppTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <App/Application.hpp>
#include <string>

using sw::app::exit_code::InvalidScenario;
using sw::app::exit_code::Success;
using namespace sw::test;

TEST(Pipeline_prints_events_of_the_example_scenario)
{
	const AppRun run = runApplication(
		"pipeline_example",
		"CREATE_MAP 10 10\n\nSPAWN_SWORDSMAN 1 0 0 5 2\n\nSPAWN_HUNTER 2 9 0 10 5 1 4\n\nMARCH 1 9 0\n\n"
		"MARCH 2 0 0\n\nSPAWN_SWORDSMAN 3 0 9 10 2\n\nMARCH 3 0 0\n");

	CHECK_EQ(run.code, Success);
	CHECK_EQ(run.err, std::string());
	CHECK_EQ(
		run.out,
		std::string("[1] MAP_CREATED width=10 height=10 \n"
					"[1] UNIT_SPAWNED unitId=1 unitType=Swordsman x=0 y=0 \n"
					"[1] UNIT_SPAWNED unitId=2 unitType=Hunter x=9 y=0 \n"
					"[1] MARCH_STARTED unitId=1 x=0 y=0 targetX=9 targetY=0 \n"
					"[1] MARCH_STARTED unitId=2 x=9 y=0 targetX=0 targetY=0 \n"
					"[1] UNIT_SPAWNED unitId=3 unitType=Swordsman x=0 y=9 \n"
					"[1] MARCH_STARTED unitId=3 x=0 y=9 targetX=0 targetY=0 \n"
					"[2] UNIT_MOVED unitId=1 x=1 y=0 \n"
					"[2] UNIT_MOVED unitId=2 x=8 y=0 \n"
					"[2] UNIT_MOVED unitId=3 x=0 y=8 \n"
					"[3] UNIT_MOVED unitId=1 x=2 y=0 \n"
					"[3] UNIT_MOVED unitId=2 x=7 y=0 \n"
					"[3] UNIT_MOVED unitId=3 x=0 y=7 \n"
					"[4] UNIT_MOVED unitId=1 x=3 y=0 \n"
					"[4] UNIT_ATTACKED attackerUnitId=2 targetUnitId=1 damage=5 targetHp=0 \n"
					"[4] UNIT_DIED unitId=1 \n"
					"[4] UNIT_MOVED unitId=3 x=0 y=6 \n"
					"[5] UNIT_MOVED unitId=2 x=6 y=0 \n"
					"[5] UNIT_MOVED unitId=3 x=0 y=5 \n"
					"[6] UNIT_MOVED unitId=2 x=5 y=0 \n"
					"[6] UNIT_MOVED unitId=3 x=0 y=4 \n"
					"[7] UNIT_MOVED unitId=2 x=4 y=0 \n"
					"[7] UNIT_MOVED unitId=3 x=0 y=3 \n"
					"[8] UNIT_ATTACKED attackerUnitId=2 targetUnitId=3 damage=5 targetHp=5 \n"
					"[8] UNIT_MOVED unitId=3 x=0 y=2 \n"
					"[9] UNIT_ATTACKED attackerUnitId=2 targetUnitId=3 damage=5 targetHp=0 \n"
					"[9] UNIT_DIED unitId=3 \n"));
}

TEST(Pipeline_reports_the_full_list_of_errors_and_prints_no_events)
{
	const AppRun run = runApplication(
		"pipeline_errors",
		"CREATE_MAP 10 10\n"
		"SPAWN_SWORDSMAN 1 0 0 5 2\n"
		"SPAWN_HUNTER 1 5 5 10 5 1 4\n"
		"SPAWN_HUNTER 2 0 0 10 5 1 1\n"
		"MARCH 9 1 1\n"
		"MARCH 1 0 0\n"
		"SPAWN_SWORDSMAN 3 10 10 5 2\n"
		"FLY 1\n");

	CHECK_EQ(run.code, InvalidScenario);
	CHECK_EQ(run.out, std::string());
	CHECK_EQ(
		run.err,
		std::string(
			"error: scenario.txt:3: SPAWN_HUNTER [unit.duplicate_id]: unit 1 already exists "
			"(unit 1 spawned at line 2)\n"
			"error: scenario.txt:4: SPAWN_HUNTER [hunter.invalid_range]: range must be >= 2, got 1\n"
			"error: scenario.txt:4: SPAWN_HUNTER [unit.cell_occupied]: cell (0,0) is occupied by unit 1 "
			"(unit 1 spawned at line 2)\n"
			"error: scenario.txt:5: MARCH [march.unit_not_found]: unit 9 does not exist\n"
			"error: scenario.txt:6: MARCH [march.same_position]: unit 1 will already be at (0,0) when this march "
			"starts (unit 1 spawned at line 2)\n"
			"error: scenario.txt:7: SPAWN_SWORDSMAN [unit.out_of_bounds]: position (10,10) is outside the map 10x10\n"
			"error: scenario.txt:8: [parse.unknown_command]: unknown command 'FLY'\n"
			"7 errors, simulation not started.\n"));
}

TEST(Pipeline_suppresses_consequences_of_earlier_errors)
{
	const AppRun brokenMap
		= runApplication("pipeline_broken_map", "CREATE_MAP 10 x\nSPAWN_SWORDSMAN 1 0 0 5 2\nMARCH 1 3 3\n");
	CHECK_EQ(brokenMap.code, InvalidScenario);
	CHECK_EQ(
		brokenMap.err,
		std::string(
			"error: scenario.txt:1: CREATE_MAP [parse.invalid_number]: height: expected non-negative integer, got "
			"\"x\"\n"
			"1 error, simulation not started.\n"));

	const AppRun rejected
		= runApplication("pipeline_rejected", "CREATE_MAP 10 10\nSPAWN_SWORDSMAN 1 0 0 0 2\nMARCH 1 3 3\n");
	CHECK_EQ(
		rejected.err,
		std::string("error: scenario.txt:2: SPAWN_SWORDSMAN [unit.invalid_stat]: hp must be positive, got 0\n"
					"1 error, simulation not started.\n"));
}

TEST(Pipeline_empty_scenario_has_no_map)
{
	const AppRun run = runApplication("pipeline_empty", "// только комментарий\n\n");
	CHECK_EQ(run.code, InvalidScenario);
	CHECK_EQ(
		run.err,
		std::string("error: scenario.txt: [map.not_created]: scenario has no CREATE_MAP command\n"
					"1 error, simulation not started.\n"));
	CHECK_EQ(run.out, std::string());
}

TEST(Pipeline_reports_stop_at_the_tick_limit)
{
	sw::app::ApplicationOptions options;
	options.tickLimit = 3;
	const AppRun run = runApplication(
		"pipeline_limit",
		"CREATE_MAP 100 1\nSPAWN_SWORDSMAN 1 0 0 5 2\nSPAWN_SWORDSMAN 2 99 0 5 2\nMARCH 1 99 0\nMARCH 2 0 0\n",
		std::move(options));

	CHECK_EQ(run.code, Success);
	CHECK_EQ(run.err, std::string("simulation stopped: tick limit (3) reached\n"));
	CHECK(run.out.find("[3] UNIT_MOVED unitId=2 x=97 y=0 \n") != std::string::npos);
	CHECK(run.out.find("[4]") == std::string::npos);
}

TEST(Pipeline_scenario_with_one_unit_prints_only_setup_events)
{
	const AppRun run = runApplication("pipeline_single", "CREATE_MAP 5 5\nSPAWN_SWORDSMAN 1 2 2 5 2\n");
	CHECK_EQ(run.code, Success);
	CHECK_EQ(
		run.out,
		std::string("[1] MAP_CREATED width=5 height=5 \n"
					"[1] UNIT_SPAWNED unitId=1 unitType=Swordsman x=2 y=2 \n"));
}
