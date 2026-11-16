#pragma once

#include "CommandRegistry.hpp"

#include <IO/System/CommandParser.hpp>
#include <istream>
#include <vector>

namespace sw::app
{
	struct LoadedScenario
	{
		std::vector<ScenarioStep> steps;
		std::vector<io::ParseDiagnostic> parseErrors;
	};

	LoadedScenario loadScenario(std::istream& input, const CommandRegistry& registry);
}
