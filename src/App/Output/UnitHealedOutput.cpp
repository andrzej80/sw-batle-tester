#include "EventBridge.hpp"

#include <App/Registration/Registrar.hpp>
#include <Core/Common/Narrow.hpp>
#include <Core/Events/CoreEvents.hpp>
#include <IO/Events/UnitHealed.hpp>
#include <cstdint>

namespace sw::app
{
	namespace
	{
		void outputUnitHealed(EventBridge& bridge)
		{
			bridge.on<core::events::UnitHealed>(
				[](const core::events::UnitHealed& e, EventLog& log, core::Tick tick)
				{
					log.log(
						tick,
						io::UnitHealed{
							e.healerId.value,
							e.targetId.value,
							core::narrow<uint32_t>(e.amount),
							core::narrow<uint32_t>(e.targetHp)});
				});
		}
	}
}

SW_REGISTER_OUTPUT(sw::app::outputUnitHealed)
