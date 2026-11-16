#include "TestFramework/TestFramework.hpp"

#include <App/Output/EventBridge.hpp>
#include <App/Registration/Registrar.hpp>
#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <IO/System/EventLog.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using sw::app::EventBridge;
using sw::core::EventBus;
using sw::core::Tick;
using sw::core::UnitId;
namespace events = sw::core::events;

namespace
{
	class CoutCapture
	{
	public:
		CoutCapture() :
				_previous(std::cout.rdbuf(_buffer.rdbuf()))
		{}

		~CoutCapture()
		{
			std::cout.rdbuf(_previous);
		}

		CoutCapture(const CoutCapture&) = delete;
		CoutCapture& operator=(const CoutCapture&) = delete;

		std::string text() const
		{
			return _buffer.str();
		}

	private:
		std::ostringstream _buffer;
		std::streambuf* _previous;
	};

	struct EventWithoutOutput
	{
		int value;
	};

	struct Output
	{
		Output()
		{
			for (const sw::app::OutputFn bind : sw::app::outputList())
			{
				bind(bridge);
			}
		}

		EventBus bus;
		sw::EventLog log;
		Tick tick = 1;
		EventBridge bridge{
			bus,
			log,
			[this]
			{
				return tick;
			}};
	};
}

TEST(CoreEventsOutput_prints_every_core_event_in_the_event_log_format)
{
	Output output;
	const CoutCapture capture;

	output.bus.publish(events::MapCreated{10, 10});
	output.bus.publish(events::UnitSpawned{UnitId{1}, "Swordsman", {0, 0}});
	output.bus.publish(events::MarchStarted{UnitId{1}, {0, 0}, {9, 0}});
	output.tick = 2;
	output.bus.publish(events::UnitMoved{UnitId{1}, {1, 0}});
	output.tick = 4;
	output.bus.publish(events::UnitAttacked{UnitId{2}, UnitId{1}, 5, 0});
	output.bus.publish(events::UnitDied{UnitId{1}});
	output.bus.publish(events::MarchEnded{UnitId{3}, {0, 0}});

	CHECK_EQ(
		capture.text(),
		std::string("[1] MAP_CREATED width=10 height=10 \n"
					"[1] UNIT_SPAWNED unitId=1 unitType=Swordsman x=0 y=0 \n"
					"[1] MARCH_STARTED unitId=1 x=0 y=0 targetX=9 targetY=0 \n"
					"[2] UNIT_MOVED unitId=1 x=1 y=0 \n"
					"[4] UNIT_ATTACKED attackerUnitId=2 targetUnitId=1 damage=5 targetHp=0 \n"
					"[4] UNIT_DIED unitId=1 \n"
					"[4] MARCH_ENDED unitId=3 x=0 y=0 \n"));
}

TEST(EventBridge_does_not_print_events_without_an_output_adapter)
{
	Output output;
	const CoutCapture capture;
	output.bus.publish(EventWithoutOutput{1});
	CHECK(capture.text().empty());
}

TEST(UnitHealedOutput_prints_the_heal_in_the_event_log_format)
{
	Output output;
	const CoutCapture capture;
	output.tick = 3;
	output.bus.publish(events::UnitHealed{UnitId{1}, UnitId{2}, 3, 7});
	CHECK_EQ(capture.text(), std::string("[3] UNIT_HEALED healerUnitId=1 targetUnitId=2 amount=3 targetHp=7 \n"));
}

TEST(UnitHealedOutput_rejects_values_outside_the_output_format)
{
	Output output;
	const CoutCapture capture;
	CHECK_THROWS(output.bus.publish(events::UnitHealed{UnitId{1}, UnitId{2}, -1, 7}), std::logic_error);
	CHECK_THROWS(output.bus.publish(events::UnitHealed{UnitId{1}, UnitId{2}, 1, 4294967296}), std::logic_error);
}

TEST(CoreEventsOutput_rejects_values_outside_the_output_format)
{
	Output output;
	const CoutCapture capture;
	CHECK_THROWS(output.bus.publish(events::MapCreated{4294967296, 1}), std::logic_error);
	CHECK_THROWS(output.bus.publish(events::UnitAttacked{UnitId{1}, UnitId{2}, -1, 0}), std::logic_error);
}

TEST(EventBridge_rejects_two_adapters_for_one_event)
{
	Output output;
	CHECK_THROWS(
		output.bridge.on<events::UnitDied>([](const events::UnitDied&, sw::EventLog&, Tick) {}), std::logic_error);
	CHECK_THROWS(
		output.bridge.on<events::UnitHealed>([](const events::UnitHealed&, sw::EventLog&, Tick) {}), std::logic_error);
	CHECK_THROWS(output.bridge.on<EventWithoutOutput>(EventBridge::Output<EventWithoutOutput>{}), std::logic_error);
}
