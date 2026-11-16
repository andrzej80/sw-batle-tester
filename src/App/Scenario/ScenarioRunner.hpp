#pragma once

#include "ScenarioLoader.hpp"

#include <App/Registration/Registrar.hpp>
#include <Core/Common/Types.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <vector>

namespace sw::core
{
	class IRandom;
}

namespace sw::app
{
	core::StopReason runScenario(
		const LoadedScenario& scenario,
		const std::vector<OutputFn>& outputs,
		core::IRandom& random,
		core::Tick tickLimit);
}
