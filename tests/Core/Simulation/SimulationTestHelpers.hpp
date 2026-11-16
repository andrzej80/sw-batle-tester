#pragma once

#include <Core/Actions/IAction.hpp>
#include <Core/Actions/MarchStep.hpp>
#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Random/IRandom.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <Core/World/World.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sw::test
{
	struct FirstRandom final : core::IRandom
	{
		size_t index(size_t count) override
		{
			if (count == 0)
			{
				throw std::logic_error("FirstRandom: empty range");
			}
			return 0;
		}
	};

	class LambdaAction final : public core::IAction
	{
	public:
		using Body = std::function<bool(const core::Unit&, core::TurnContext&)>;

		explicit LambdaAction(Body body) :
				_body(std::move(body))
		{}

		bool tryExecute(const core::Unit& self, core::TurnContext& context) override
		{
			return _body(self, context);
		}

	private:
		Body _body;
	};

	struct SimUnit
	{
		uint32_t id;
		core::Position position;
		std::optional<int64_t> speed = 1;
		bool occupiesCell = true;
		std::optional<int64_t> hp = 10;
		std::vector<LambdaAction::Body> actions = {};
		bool marches = true;
	};

	struct SimArena
	{
		SimArena(int64_t width, int64_t height) :
				world(width, height),
				simulation(world, bus, random, 30'000)
		{
			subscribeLog();
		}

		SimArena(int64_t width, int64_t height, core::Tick tickLimit) :
				world(width, height),
				simulation(world, bus, random, tickLimit)
		{
			subscribeLog();
		}

		SimArena(const SimArena&) = delete;
		SimArena& operator=(const SimArena&) = delete;

		core::Unit& spawn(const SimUnit& description)
		{
			core::UnitBlueprint blueprint;
			blueprint.id = core::UnitId{description.id};
			blueprint.type = "TestUnit";
			blueprint.position = description.position;
			blueprint.occupiesCell = description.occupiesCell;
			if (description.hp)
			{
				blueprint.components.add<core::Health>(*description.hp);
			}
			if (description.speed)
			{
				blueprint.components.add<core::Movement>(*description.speed);
			}
			for (const LambdaAction::Body& body : description.actions)
			{
				blueprint.behaviour.push_back(std::make_unique<LambdaAction>(body));
			}
			if (description.marches && description.speed)
			{
				blueprint.behaviour.push_back(std::make_unique<core::MarchStep>());
			}
			return world.place(core::Unit(std::move(blueprint)));
		}

		void march(const core::Unit& unit, core::Position target)
		{
			simulation.context().movement().addMarch(unit, target);
		}

		core::TurnContext& context()
		{
			return simulation.context();
		}

		core::World world;
		core::EventBus bus;
		FirstRandom random;
		core::Simulation simulation;
		std::vector<std::string> log;

	private:
		std::string at() const
		{
			return "[" + std::to_string(world.tick()) + "] ";
		}

		static std::string cell(core::Position position)
		{
			return std::to_string(position.x) + "," + std::to_string(position.y);
		}

		void subscribeLog()
		{
			namespace events = core::events;
			bus.subscribe<events::UnitMoved>(
				[this](const events::UnitMoved& e)
				{ log.push_back(at() + "moved " + std::to_string(e.unitId.value) + " " + cell(e.position)); });
			bus.subscribe<events::MarchStarted>(
				[this](const events::MarchStarted& e)
				{
					log.push_back(
						at() + "march started " + std::to_string(e.unitId.value) + " " + cell(e.from) + "->"
						+ cell(e.target));
				});
			bus.subscribe<events::MarchEnded>(
				[this](const events::MarchEnded& e)
				{ log.push_back(at() + "march ended " + std::to_string(e.unitId.value) + " " + cell(e.position)); });
			bus.subscribe<events::UnitDied>([this](const events::UnitDied& e)
											{ log.push_back(at() + "died " + std::to_string(e.unitId.value)); });
		}
	};
}
