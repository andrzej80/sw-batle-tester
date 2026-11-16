#pragma once

#include "ScenarioLoader.hpp"

#include <App/DiagnosticPrinter.hpp>
#include <vector>

namespace sw::app
{
	std::vector<ScenarioError> validateScenario(const LoadedScenario& scenario);
}
