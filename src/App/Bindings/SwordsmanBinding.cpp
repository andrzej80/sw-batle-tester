#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Features/Units/Swordsman.hpp>
#include <IO/Commands/SpawnSwordsman.hpp>

namespace sw::app
{
	namespace
	{
		void bindSwordsman(CommandRegistry& registry)
		{
			registry.on<io::SpawnSwordsman>(
				[](const io::SpawnSwordsman& command, core::SetupContext& setup)
				{
					return setup.spawn(features::makeSwordsman(
						{core::UnitId{command.unitId}, {command.x, command.y}, command.hp, command.strength}));
				});
		}
	}
}

SW_REGISTER_BINDING(sw::app::bindSwordsman)
