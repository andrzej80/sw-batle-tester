#include "EventBridge.hpp"

#include <App/Registration/Registrar.hpp>
#include <Features/Units/Mine.hpp>
#include <IO/Events/MineExploded.hpp>

namespace sw::app
{
	namespace
	{
		void outputMineExploded(EventBridge& bridge)
		{
			bridge.on<features::MineExploded>([](const features::MineExploded& e, EventLog& log, core::Tick tick)
											  { log.log(tick, io::MineExploded{e.unitId.value}); });
		}
	}
}

SW_REGISTER_OUTPUT(sw::app::outputMineExploded)
