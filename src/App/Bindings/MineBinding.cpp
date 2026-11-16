#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Features/Units/Mine.hpp>
#include <IO/Commands/SpawnMine.hpp>

namespace sw::app
{
	namespace
	{
		void bindMine(CommandRegistry& registry)
		{
			registry.on<io::SpawnMine>(
				[](const io::SpawnMine& command, core::SetupContext& setup) {
					return setup.spawn(
						features::makeMine({core::UnitId{command.unitId}, {command.x, command.y}, command.power}));
				});
		}
	}
}

SW_REGISTER_BINDING(sw::app::bindMine)
