#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Features/Units/Raven.hpp>
#include <IO/Commands/SpawnRaven.hpp>

namespace sw::app
{
	namespace
	{
		void bindRaven(CommandRegistry& registry)
		{
			registry.on<io::SpawnRaven>(
				[](const io::SpawnRaven& command, core::SetupContext& setup)
				{
					return setup.spawn(features::makeRaven(
						{core::UnitId{command.unitId}, {command.x, command.y}, command.hp, command.agility}));
				});
		}
	}
}

SW_REGISTER_BINDING(sw::app::bindRaven)
