#include "Application.hpp"

#include "DiagnosticPrinter.hpp"
#include "Scenario/CommandRegistry.hpp"
#include "Scenario/ScenarioLoader.hpp"
#include "Scenario/ScenarioRunner.hpp"
#include "Scenario/ScenarioValidator.hpp"

#include <exception>
#include <fstream>
#include <ostream>
#include <utility>

namespace sw::app
{
	Application::Application(ApplicationOptions options) :
			_options(std::move(options))
	{}

	int Application::run(int argc, const char* const* argv, std::ostream& errors) const
	{
		try
		{
			if (argc != 2)
			{
				errors << "usage: sw_battle_test <scenario-file>\n";
				return exit_code::Environment;
			}
			return processFile(argv[1], errors);
		}
		catch (const std::exception& exception)
		{
			errors << "internal error: " << exception.what() << '\n';
			return exit_code::InternalError;
		}
		catch (...)
		{
			errors << "internal error: unknown exception\n";
			return exit_code::InternalError;
		}
	}

	int Application::processFile(const std::string& path, std::ostream& errors) const
	{
		std::ifstream file(path);
		if (!file)
		{
			errors << "error: cannot open file '" << path << "'\n";
			return exit_code::Environment;
		}

		CommandRegistry registry;
		for (const BindingFn bind : _options.bindings)
		{
			bind(registry);
		}
		const LoadedScenario scenario = loadScenario(file, registry);

		std::vector<ScenarioError> problems = validateScenario(scenario);
		if (!problems.empty())
		{
			printScenarioErrors(errors, path, std::move(problems));
			return exit_code::InvalidScenario;
		}

		const std::unique_ptr<core::IRandom> random = _options.random();
		const core::StopReason reason = runScenario(scenario, _options.outputs, *random, _options.tickLimit);
		if (reason == core::StopReason::TickLimit)
		{
			errors << "simulation stopped: tick limit (" << _options.tickLimit << ") reached\n";
		}
		return exit_code::Success;
	}
}
