#include "ScenarioValidator.hpp"

#include <Core/Common/Diagnostic.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Core/World/World.hpp>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <unordered_map>

namespace sw::app
{
	namespace
	{
		using SpawnLines = std::unordered_map<core::UnitId, size_t>;

		ScenarioError located(
			const core::Diagnostic& diagnostic,
			std::optional<size_t> line,
			const std::string& command,
			const SpawnLines& spawnLines)
		{
			ScenarioError error{line, command, diagnostic.code, diagnostic.message, {}, {}};
			if (diagnostic.relatedUnit)
			{
				error.relatedUnit = diagnostic.relatedUnit->value;
				if (const auto found = spawnLines.find(*diagnostic.relatedUnit); found != spawnLines.end())
				{
					error.relatedLine = found->second;
				}
			}
			return error;
		}

		void rememberSpawns(core::SetupContext& setup, size_t line, SpawnLines& spawnLines)
		{
			const core::World* world = setup.world();
			if (world == nullptr)
			{
				return;
			}
			for (const core::UnitId id : world->units().orderedIds())
			{
				spawnLines.try_emplace(id, line);
			}
		}
	}

	std::vector<ScenarioError> validateScenario(const LoadedScenario& scenario)
	{
		std::vector<ScenarioError> errors;
		errors.reserve(scenario.parseErrors.size());
		for (const io::ParseDiagnostic& diagnostic : scenario.parseErrors)
		{
			errors.push_back({diagnostic.line, diagnostic.command, diagnostic.code, diagnostic.message, {}, {}});
		}

		const core::EventBus quiet;
		core::SetupContext setup(quiet);
		const bool mapLineBroken = std::any_of(
			scenario.parseErrors.begin(),
			scenario.parseErrors.end(),
			[](const io::ParseDiagnostic& diagnostic) { return diagnostic.command == "CREATE_MAP"; });
		if (mapLineBroken)
		{
			setup.noteFailedMapCommand();
		}

		SpawnLines spawnLines;
		for (const ScenarioStep& step : scenario.steps)
		{
			for (const core::Diagnostic& diagnostic : step.execute(setup).errors())
			{
				errors.push_back(located(diagnostic, step.line, step.command, spawnLines));
			}
			rememberSpawns(setup, step.line, spawnLines);
		}
		for (const core::Diagnostic& diagnostic : setup.finish().errors())
		{
			errors.push_back(located(diagnostic, std::nullopt, "", spawnLines));
		}
		return errors;
	}
}
