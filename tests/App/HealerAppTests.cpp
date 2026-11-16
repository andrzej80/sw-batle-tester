#include "App/AppTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <App/Application.hpp>
#include <string>

using namespace sw::test;

TEST(SpawnHealer_spawns_a_healer)
{
	const AppRun run = runApplication("healer_spawn", "CREATE_MAP 10 10\nSPAWN_HEALER 1 3 4 10 2\n");

	CHECK_EQ(run.code, sw::app::exit_code::Success);
	CHECK_EQ(run.err, std::string());
	CHECK(run.out.find("[1] UNIT_SPAWNED unitId=1 unitType=Healer x=3 y=4") != std::string::npos);
}

TEST(SpawnHealer_reports_parse_and_stat_errors)
{
	const AppRun run = runApplication(
		"healer_errors",
		"CREATE_MAP 10 10\n"
		"SPAWN_HEALER 1 0 0 10\n"
		"SPAWN_HEALER 2 0 0 10 2 7\n"
		"SPAWN_HEALER 3 1 1 0 0\n");

	CHECK_EQ(run.code, sw::app::exit_code::InvalidScenario);
	CHECK_EQ(
		run.err,
		std::string("error: scenario.txt:2: SPAWN_HEALER [parse.missing_argument]: missing argument 'spirit'\n"
					"error: scenario.txt:3: SPAWN_HEALER [parse.extra_argument]: unexpected extra argument '7'\n"
					"error: scenario.txt:4: SPAWN_HEALER [unit.invalid_stat]: hp must be positive, got 0\n"
					"error: scenario.txt:4: SPAWN_HEALER [unit.invalid_stat]: spirit must be positive, got 0\n"
					"4 errors, simulation not started.\n"));
}
