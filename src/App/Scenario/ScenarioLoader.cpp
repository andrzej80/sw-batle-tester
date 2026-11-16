#include "ScenarioLoader.hpp"

namespace sw::app
{
	LoadedScenario loadScenario(std::istream& input, const CommandRegistry& registry)
	{
		LoadedScenario scenario;
		io::CommandParser parser;
		registry.installInto(parser, scenario.steps);
		scenario.parseErrors = parser.parse(input);
		return scenario;
	}
}
