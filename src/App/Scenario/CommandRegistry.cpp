#include "CommandRegistry.hpp"

namespace sw::app
{
	void CommandRegistry::installInto(io::CommandParser& parser, std::vector<ScenarioStep>& steps) const
	{
		for (const Installer& install : _installers)
		{
			install(parser, steps);
		}
	}
}
