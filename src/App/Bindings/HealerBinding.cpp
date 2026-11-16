#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Features/Units/Healer.hpp>
#include <IO/Commands/SpawnHealer.hpp>

namespace sw::app
{
	namespace
	{
		void bindHealer(CommandRegistry& registry)
		{
			registry.on<io::SpawnHealer>(
				[](const io::SpawnHealer& command, core::SetupContext& setup)
				{
					return setup.spawn(features::makeHealer(
						{core::UnitId{command.unitId}, {command.x, command.y}, command.hp, command.spirit}));
				});
		}
	}
}

SW_REGISTER_BINDING(sw::app::bindHealer)
