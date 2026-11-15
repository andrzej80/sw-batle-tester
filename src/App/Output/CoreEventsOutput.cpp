// Адаптеры событий Core: доменные события → io::Events → EventLog (SPECIFICATION.md,
// п. 10.4, 10.5). Файл меняет владелец Core: набор событий Core закрыт намеренно.
// Числа домена — int64_t, формата вывода — uint32_t: сужение только через narrow, значение
// вне диапазона — баг (код выхода 3), а не тихо искажённый вывод.
// UnitHealed не выводится: его DTO появится в IO вместе с Лекарем.

#include "EventBridge.hpp"

#include <App/Registration/Registrar.hpp>
#include <Core/Common/Narrow.hpp>
#include <Core/Events/CoreEvents.hpp>
#include <IO/Events/MapCreated.hpp>
#include <IO/Events/MarchEnded.hpp>
#include <IO/Events/MarchStarted.hpp>
#include <IO/Events/UnitAttacked.hpp>
#include <IO/Events/UnitDied.hpp>
#include <IO/Events/UnitMoved.hpp>
#include <IO/Events/UnitSpawned.hpp>
#include <cstdint>

namespace sw::app
{
	namespace
	{
		namespace events = core::events;
		using core::narrow;
		using core::Tick;

		uint32_t out(int64_t value)
		{
			return narrow<uint32_t>(value);
		}

		void outputCoreEvents(EventBridge& bridge)
		{
			bridge
				.on<events::MapCreated>([](const events::MapCreated& e, EventLog& log, Tick tick)
										{ log.log(tick, io::MapCreated{out(e.width), out(e.height)}); })
				.on<events::UnitSpawned>(
					[](const events::UnitSpawned& e, EventLog& log, Tick tick) {
						log.log(
							tick, io::UnitSpawned{e.unitId.value, e.unitType, out(e.position.x), out(e.position.y)});
					})
				.on<events::UnitMoved>(
					[](const events::UnitMoved& e, EventLog& log, Tick tick)
					{ log.log(tick, io::UnitMoved{e.unitId.value, out(e.position.x), out(e.position.y)}); })
				.on<events::MarchStarted>(
					[](const events::MarchStarted& e, EventLog& log, Tick tick) {
						log.log(
							tick,
							io::MarchStarted{
								e.unitId.value, out(e.from.x), out(e.from.y), out(e.target.x), out(e.target.y)});
					})
				.on<events::MarchEnded>(
					[](const events::MarchEnded& e, EventLog& log, Tick tick)
					{ log.log(tick, io::MarchEnded{e.unitId.value, out(e.position.x), out(e.position.y)}); })
				.on<events::UnitAttacked>(
					[](const events::UnitAttacked& e, EventLog& log, Tick tick) {
						log.log(
							tick,
							io::UnitAttacked{e.attackerId.value, e.targetId.value, out(e.damage), out(e.targetHp)});
					})
				.on<events::UnitDied>([](const events::UnitDied& e, EventLog& log, Tick tick)
									  { log.log(tick, io::UnitDied{e.unitId.value}); });
		}
	}
}

SW_REGISTER_OUTPUT(sw::app::outputCoreEvents)
