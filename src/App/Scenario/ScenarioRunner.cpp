#include "ScenarioRunner.hpp"

#include <App/Output/EventBridge.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Core/World/World.hpp>
#include <IO/System/EventLog.hpp>
#include <stdexcept>
#include <string>

namespace sw::app
{
	core::StopReason runScenario(
		const LoadedScenario& scenario,
		const std::vector<OutputFn>& outputs,
		core::IRandom& random,
		core::Tick tickLimit)
	{
		core::EventBus bus;
		core::SetupContext setup(bus);
		EventLog log;
		// До CREATE_MAP мира нет, но и событий тоже: все команды сценария — тик 1
		EventBridge bridge(bus, log, [&setup] { return setup.world() != nullptr ? setup.world()->tick() : 1; });
		for (const OutputFn bind : outputs)
		{
			bind(bridge);
		}

		for (const ScenarioStep& step : scenario.steps)
		{
			if (!step.execute(setup).ok())
			{
				throw std::logic_error(
					"ScenarioRunner: line " + std::to_string(step.line) + " failed although validation accepted it");
			}
		}
		if (setup.world() == nullptr || !setup.finish().ok())
		{
			throw std::logic_error("ScenarioRunner: no map although validation accepted the scenario");
		}

		core::Simulation simulation(*setup.world(), bus, random, tickLimit);
		return simulation.run();
	}
}
