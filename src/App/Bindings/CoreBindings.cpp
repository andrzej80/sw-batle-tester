// Команды механик Core: CREATE_MAP и MARCH → SetupContext (SPECIFICATION.md, п. 10.5).
// Адаптеры без логики: только перевод полей uint32_t формата в типы домена (расширение
// без потерь).

#include <App/Registration/Registrar.hpp>
#include <App/Scenario/CommandRegistry.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <IO/Commands/CreateMap.hpp>
#include <IO/Commands/March.hpp>

namespace sw::app
{
	namespace
	{
		void bindCoreCommands(CommandRegistry& registry)
		{
			registry
				.on<io::CreateMap>([](const io::CreateMap& command, core::SetupContext& setup)
								   { return setup.createMap(command.width, command.height); })
				.on<io::March>(
					[](const io::March& command, core::SetupContext& setup)
					{ return setup.orderMarch(core::UnitId{command.unitId}, {command.targetX, command.targetY}); });
		}
	}
}

SW_REGISTER_BINDING(sw::app::bindCoreCommands)
