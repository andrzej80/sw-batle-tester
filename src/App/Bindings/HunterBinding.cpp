// SPAWN_HUNTER → фабрика Охотника. Единственное место, где встречаются IO и фича.

#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Features/Units/Hunter.hpp>
#include <IO/Commands/SpawnHunter.hpp>

namespace sw::app
{
	namespace
	{
		void bindHunter(CommandRegistry& registry)
		{
			registry.on<io::SpawnHunter>(
				[](const io::SpawnHunter& command, core::SetupContext& setup)
				{
					return setup.spawn(features::makeHunter(
						{core::UnitId{command.unitId},
						 {command.x, command.y},
						 command.hp,
						 command.agility,
						 command.strength,
						 command.range}));
				});
		}
	}
}

SW_REGISTER_BINDING(sw::app::bindHunter)
