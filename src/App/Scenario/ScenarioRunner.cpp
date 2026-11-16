#include "ScenarioRunner.hpp"

#include <App/Output/EventBridge.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Core/World/World.hpp>
#include <IO/System/EventLog.hpp>
#include <memory>
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
		std::unique_ptr<core::World> world;
		// Все команды сценария выполняются в тике 1, дальше тик ведёт мир симуляции
		EventBridge bridge(bus, log, [&world] { return world ? world->tick() : core::Tick{1}; });
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
		if (!setup.finish().ok())
		{
			throw std::logic_error("ScenarioRunner: setup failed although validation accepted the scenario");
		}
		world = setup.takeWorld();
		if (!world)
		{
			throw std::logic_error("ScenarioRunner: no map although validation accepted the scenario");
		}

		core::Simulation simulation(*world, bus, random, tickLimit);
		return simulation.run();
	}
}
