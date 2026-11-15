#include "Application.hpp"

#include "DiagnosticPrinter.hpp"
#include "Scenario/CommandRegistry.hpp"
#include "Scenario/ScenarioLoader.hpp"

#include <exception>
#include <fstream>
#include <ostream>
#include <utility>

namespace sw::app
{
	namespace
	{
		std::vector<ScenarioError> toScenarioErrors(const std::vector<io::ParseDiagnostic>& diagnostics)
		{
			std::vector<ScenarioError> errors;
			errors.reserve(diagnostics.size());
			for (const io::ParseDiagnostic& diagnostic : diagnostics)
			{
				errors.push_back({diagnostic.line, diagnostic.command, diagnostic.code, diagnostic.message, {}, {}});
			}
			return errors;
		}
	}

	Application::Application(std::vector<BindingFn> bindings) :
			_bindings(std::move(bindings))
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
			return runScenario(argv[1], errors);
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

	int Application::runScenario(const std::string& path, std::ostream& errors) const
	{
		std::ifstream file(path);
		if (!file)
		{
			errors << "error: cannot open file '" << path << "'\n";
			return exit_code::Environment;
		}

		CommandRegistry registry;
		for (const BindingFn bind : _bindings)
		{
			bind(registry);
		}

		const LoadedScenario scenario = loadScenario(file, registry);
		if (!scenario.parseErrors.empty())
		{
			printScenarioErrors(errors, path, toScenarioErrors(scenario.parseErrors));
			return exit_code::InvalidScenario;
		}

		// Проход проверки и запуск симуляции подключаются в задаче T15
		errors << "sw_battle_test: simulation is not connected yet\n";
		return exit_code::Success;
	}
}
