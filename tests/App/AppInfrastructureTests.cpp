#include "App/AppTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <App/Application.hpp>
#include <App/DiagnosticPrinter.hpp>
#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <App/Scenario/ScenarioLoader.hpp>
#include <Core/Events/EventBus.hpp>
#include <IO/Commands/CreateMap.hpp>
#include <IO/Commands/March.hpp>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using sw::app::Application;
using sw::app::BindingFn;
using sw::app::CommandRegistry;
using sw::app::ScenarioError;
using sw::app::ScenarioStep;
using sw::core::Result;
using sw::core::SetupContext;
using namespace sw::test;

namespace
{
	std::vector<std::string> calls;

	void bindCreateMap(CommandRegistry& registry)
	{
		registry.on<sw::io::CreateMap>(
			[](const sw::io::CreateMap& command, SetupContext&) -> Result<void>
			{
				calls.push_back("CREATE_MAP " + std::to_string(command.width) + "x" + std::to_string(command.height));
				return {};
			});
	}

	void bindMarch(CommandRegistry& registry)
	{
		registry.on<sw::io::March>(
			[](const sw::io::March& command, SetupContext&) -> Result<void>
			{
				calls.push_back("MARCH " + std::to_string(command.unitId));
				return {};
			});
	}

	struct ProbeCommand
	{
		constexpr static const char* Name = "SW_TEST_PROBE";

		template <typename Visitor>
		void visit(Visitor&)
		{}
	};

	void bindProbe(CommandRegistry& registry)
	{
		registry.on<ProbeCommand>([](const ProbeCommand&, SetupContext&) -> Result<void> { return {}; });
	}

	int runApp(const std::vector<BindingFn>& bindings, const std::vector<std::string>& arguments, std::string& errors)
	{
		std::vector<const char*> argv{"sw_battle_test"};
		for (const std::string& argument : arguments)
		{
			argv.push_back(argument.c_str());
		}
		std::ostringstream stream;
		const int code = Application({.bindings = bindings}).run(static_cast<int>(argv.size()), argv.data(), stream);
		errors = stream.str();
		return code;
	}
}

SW_REGISTER_BINDING(bindProbe)

TEST(Registrar_macro_adds_binding_to_the_shared_list)
{
	const std::vector<BindingFn>& bindings = sw::app::bindingList();
	CHECK(std::find(bindings.begin(), bindings.end(), &bindProbe) != bindings.end());
}

TEST(CommandRegistry_rejects_command_registered_twice)
{
	CommandRegistry registry;
	bindMarch(registry);
	CHECK_THROWS(bindMarch(registry), std::logic_error);
}

TEST(ScenarioLoader_turns_lines_into_repeatable_steps)
{
	CommandRegistry registry;
	bindCreateMap(registry);
	bindMarch(registry);
	std::istringstream input("CREATE_MAP 10 5\n\nMARCH 7 1 1\nFLY 1\nMARCH 1\n");

	const sw::app::LoadedScenario scenario = sw::app::loadScenario(input, registry);

	CHECK_EQ(scenario.steps.size(), size_t{2});
	CHECK_EQ(scenario.steps[0].line, size_t{1});
	CHECK_EQ(scenario.steps[0].command, std::string("CREATE_MAP"));
	CHECK_EQ(scenario.steps[1].line, size_t{3});
	CHECK_EQ(scenario.parseErrors.size(), size_t{2});
	CHECK_EQ(scenario.parseErrors[0].code, std::string("parse.unknown_command"));
	CHECK_EQ(scenario.parseErrors[1].code, std::string("parse.missing_argument"));

	calls.clear();
	sw::core::EventBus bus;
	SetupContext first(bus);
	SetupContext second(bus);
	for (const ScenarioStep& step : scenario.steps)
	{
		CHECK(step.execute(first).ok());
		CHECK(step.execute(second).ok());
	}
	CHECK(calls == (std::vector<std::string>{"CREATE_MAP 10x5", "CREATE_MAP 10x5", "MARCH 7", "MARCH 7"}));
}

TEST(DiagnosticPrinter_prints_errors_in_line_order_with_summary)
{
	std::ostringstream output;
	sw::app::printScenarioErrors(
		output,
		"scenario.txt",
		{
			{9, "MARCH", "march.unit_not_found", "unit 42 does not exist", {}, {}},
			{std::nullopt, "", "map.not_created", "the map was never created", {}, {}},
			{7, "SPAWN_HUNTER", "unit.cell_occupied", "cell (9,0) is occupied by unit 2", 2, 5},
			{2, "", "parse.unknown_command", "unknown command 'FLY'", {}, {}},
			{7, "SPAWN_HUNTER", "hunter.invalid_range", "range must be >= 2, got 1", {}, {}},
		});

	CHECK_EQ(
		output.str(),
		std::string("error: scenario.txt:2: [parse.unknown_command]: unknown command 'FLY'\n"
					"error: scenario.txt:7: SPAWN_HUNTER [unit.cell_occupied]: cell (9,0) is occupied by unit 2 "
					"(unit 2 spawned at line 5)\n"
					"error: scenario.txt:7: SPAWN_HUNTER [hunter.invalid_range]: range must be >= 2, got 1\n"
					"error: scenario.txt:9: MARCH [march.unit_not_found]: unit 42 does not exist\n"
					"error: scenario.txt: [map.not_created]: the map was never created\n"
					"5 errors, simulation not started.\n"));
}

TEST(DiagnosticPrinter_uses_singular_for_one_error)
{
	std::ostringstream output;
	sw::app::printScenarioErrors(
		output, "a.txt", {{1, "MARCH", "parse.extra_argument", "unexpected extra argument '4'", {}, {}}});
	CHECK(output.str().find("1 error, simulation not started.") != std::string::npos);
}

TEST(Application_without_exactly_one_argument_prints_usage)
{
	std::string errors;
	CHECK_EQ(runApp({}, {}, errors), sw::app::exit_code::Environment);
	CHECK_EQ(errors, std::string("usage: sw_battle_test <scenario-file>\n"));
	CHECK_EQ(runApp({}, {"a.txt", "b.txt"}, errors), sw::app::exit_code::Environment);
}

TEST(Application_reports_file_that_cannot_be_opened)
{
	std::string errors;
	CHECK_EQ(runApp({}, {"/nonexistent/scenario.txt"}, errors), sw::app::exit_code::Environment);
	CHECK_EQ(errors, std::string("error: cannot open file '/nonexistent/scenario.txt'\n"));
}

TEST(Application_prints_all_parse_errors_and_does_not_start)
{
	const TempScenario scenario("parse_errors", "CREATE_MAP 10 x\nMARCH 1 2 3\nFLY\n");
	std::string errors;
	calls.clear();

	CHECK_EQ(runApp({&bindCreateMap, &bindMarch}, {scenario.path()}, errors), sw::app::exit_code::InvalidScenario);
	CHECK(
		errors.find(":1: CREATE_MAP [parse.invalid_number]: height: expected non-negative integer")
		!= std::string::npos);
	CHECK(errors.find(":3: [parse.unknown_command]: unknown command 'FLY'") != std::string::npos);
	CHECK(errors.find("2 errors, simulation not started.") != std::string::npos);
	CHECK(calls == (std::vector<std::string>{"MARCH 1"}));
}

TEST(Application_turns_internal_errors_into_exit_code_three)
{
	const TempScenario scenario("internal_error", "MARCH 1 2 3\n");
	std::string errors;
	CHECK_EQ(runApp({&bindMarch, &bindMarch}, {scenario.path()}, errors), sw::app::exit_code::InternalError);
	CHECK(errors.find("internal error: CommandRegistry: command MARCH is registered twice") != std::string::npos);
}
