#include "App/AppTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <App/Application.hpp>
#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <App/Scenario/ScenarioLoader.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/World/World.hpp>
#include <sstream>
#include <string>
#include <vector>

using sw::app::CommandRegistry;
using sw::app::LoadedScenario;
using sw::app::ScenarioStep;
using sw::core::UnitId;
using namespace sw::test;

namespace
{
	CommandRegistry realRegistry()
	{
		CommandRegistry registry;
		for (const sw::app::BindingFn bind : sw::app::bindingList())
		{
			bind(registry);
		}
		return registry;
	}

	LoadedScenario load(const std::string& text)
	{
		std::istringstream input(text);
		return sw::app::loadScenario(input, realRegistry());
	}

	const std::string example
		= "CREATE_MAP 10 10\n"
		  "SPAWN_SWORDSMAN 1 0 0 5 2\n"
		  "SPAWN_HUNTER 2 9 0 10 5 1 4\n"
		  "MARCH 1 9 0\n"
		  "MARCH 2 0 0\n"
		  "SPAWN_SWORDSMAN 3 0 9 10 2\n"
		  "MARCH 3 0 0\n";
}

TEST(Bindings_register_all_scenario_commands)
{
	const LoadedScenario scenario = load(example);
	CHECK(scenario.parseErrors.empty());
	CHECK_EQ(scenario.steps.size(), size_t{7});
}

TEST(Bindings_build_the_world_of_the_example_scenario)
{
	const LoadedScenario scenario = load(example);
	sw::core::EventBus bus;
	sw::core::SetupContext setup(bus);
	for (const ScenarioStep& step : scenario.steps)
	{
		CHECK(step.execute(setup).ok());
	}

	const sw::core::World* world = setup.world();
	CHECK(world != nullptr);
	CHECK_EQ(world->map().width(), int64_t{10});
	CHECK_EQ(world->units().size(), size_t{3});
	CHECK_EQ(world->units().find(UnitId{1})->type(), std::string_view("Swordsman"));
	CHECK_EQ(world->units().find(UnitId{2})->type(), std::string_view("Hunter"));
	CHECK_EQ(world->units().find(UnitId{2})->position(), (sw::core::Position{9, 0}));
	CHECK_EQ(world->units().find(UnitId{3})->get<sw::core::Movement>().marches.size(), size_t{1});
}

TEST(Bindings_pass_feature_errors_through)
{
	const LoadedScenario scenario = load("CREATE_MAP 10 10\nSPAWN_HUNTER 2 0 0 0 5 1 1\nSPAWN_SWORDSMAN 3 10 0 5 2\n");
	sw::core::EventBus bus;
	sw::core::SetupContext setup(bus);
	std::vector<std::string> codes;
	for (const ScenarioStep& step : scenario.steps)
	{
		for (const sw::core::Diagnostic& error : step.execute(setup).errors())
		{
			codes.push_back(std::to_string(step.line) + " " + error.code);
		}
	}
	CHECK(codes == (std::vector<std::string>{"2 unit.invalid_stat", "2 hunter.invalid_range", "3 unit.out_of_bounds"}));
}

TEST(Application_with_real_bindings_accepts_the_example_scenario)
{
	const TempScenario scenario("example_bindings", example);
	const std::string path = scenario.path();
	const char* argv[] = {"sw_battle_test", path.c_str()};
	std::ostringstream errors;

	CHECK_EQ(sw::app::Application{}.run(2, argv, errors), sw::app::exit_code::Success);
	CHECK(errors.str().find("error") == std::string::npos);
}
