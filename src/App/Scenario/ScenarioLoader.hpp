#pragma once

#include "CommandRegistry.hpp"

#include <IO/System/CommandParser.hpp>
#include <istream>
#include <vector>

namespace sw::app
{
	struct LoadedScenario
	{
		std::vector<ScenarioStep> steps;  // команды без ошибок разбора, по порядку строк
		std::vector<io::ParseDiagnostic> parseErrors;  // ошибки разбора всех строк
	};

	// Читает сценарий целиком: команды, которые разобрались, и все ошибки разбора
	LoadedScenario loadScenario(std::istream& input, const CommandRegistry& registry);
}
