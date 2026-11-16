#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Features/Units/Tower.hpp>
#include <IO/Commands/SpawnTower.hpp>

namespace sw::app
{
	namespace
	{
		void bindTower(CommandRegistry& registry)
		{
			registry.on<io::SpawnTower>(
				[](const io::SpawnTower& command, core::SetupContext& setup)
				{
					return setup.spawn(features::makeTower(
						{core::UnitId{command.unitId}, {command.x, command.y}, command.hp, command.power}));
				});
		}
	}
}

SW_REGISTER_BINDING(sw::app::bindTower)
