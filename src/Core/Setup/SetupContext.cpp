#include "SetupContext.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Simulation/MovementService.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/World/World.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace sw::core
{
	namespace
	{
		Result<void> toResult(Diagnostics errors)
		{
			if (errors.empty())
			{
				return {};
			}
			return errors;
		}

		std::string describe(Position position)
		{
			std::ostringstream stream;
			stream << position;
			return stream.str();
		}

		std::string mapSize(const World& world)
		{
			return std::to_string(world.map().width()) + "x" + std::to_string(world.map().height());
		}

		std::string unitName(UnitId id)
		{
			return "unit " + std::to_string(id.value);
		}
	}

	SetupContext::SetupContext(const EventBus& events) :
			_events(events)
	{}

	SetupContext::~SetupContext() = default;

	bool SetupContext::requireMap(Diagnostics& errors)
	{
		if (_world)
		{
			return true;
		}
		// После CREATE_MAP с ошибкой первопричина уже в списке
		if (!_mapCommandSeen && !_missingMapReported)
		{
			errors.add("map.not_created", "CREATE_MAP must come before other commands");
			_missingMapReported = true;
		}
		return false;
	}

	Result<void> SetupContext::createMap(int64_t width, int64_t height)
	{
		Diagnostics errors;
		if (_mapCommandSeen)
		{
			errors.add("map.already_created", "CREATE_MAP may appear only once");
		}
		if (width <= 0 || height <= 0)
		{
			errors.add(
				"map.invalid_size",
				"map size must be positive, got " + std::to_string(width) + "x" + std::to_string(height));
		}
		_mapCommandSeen = true;
		if (!errors.empty())
		{
			return errors;
		}

		_world = std::make_unique<World>(width, height);
		_movement = std::make_unique<MovementService>(*_world, _events);
		_events.publish(events::MapCreated{width, height});
		return {};
	}

	Result<void> SetupContext::spawn(SpawnRequest request)
	{
		Diagnostics errors;
		errors.append(request.unit.errors());

		if (!requireMap(errors))
		{
			_rejected.insert(request.id);
			return toResult(std::move(errors));
		}

		const bool duplicate = _world->units().find(request.id) != nullptr;
		if (duplicate)
		{
			errors.add("unit.duplicate_id", unitName(request.id) + " already exists", request.id);
		}
		if (!_world->map().contains(request.position))
		{
			errors.add(
				"unit.out_of_bounds",
				"position " + describe(request.position) + " is outside the map " + mapSize(*_world));
		}
		else if (request.occupiesCell)
		{
			if (const auto occupant = _world->map().occupant(request.position))
			{
				errors.add(
					"unit.cell_occupied",
					"cell " + describe(request.position) + " is occupied by " + unitName(*occupant),
					occupant);
			}
		}

		if (!errors.empty())
		{
			// Повторный id принадлежит уже стоящему юниту: его марши проверяются как обычно
			if (!duplicate)
			{
				_rejected.insert(request.id);
			}
			return errors;
		}

		Unit unit = std::move(request.unit).value();
		if (unit.id() != request.id || unit.position() != request.position
			|| unit.occupiesCell() != request.occupiesCell)
		{
			throw std::logic_error("SetupContext: spawn request does not match the assembled " + unitName(request.id));
		}
		const Unit& placed = _world->place(std::move(unit));
		_events.publish(events::UnitSpawned{placed.id(), std::string(placed.type()), placed.position()});
		return {};
	}

	Result<void> SetupContext::orderMarch(UnitId id, Position target)
	{
		Diagnostics errors;
		if (!requireMap(errors) || _rejected.contains(id))
		{
			return toResult(std::move(errors));
		}

		const Unit* unit = _world->units().find(id);
		if (unit == nullptr)
		{
			errors.add("march.unit_not_found", unitName(id) + " does not exist");
			return errors;
		}
		const auto* movement = unit->tryGet<Movement>();
		if (movement == nullptr)
		{
			errors.add("march.unit_cannot_move", unitName(id) + " (" + std::string(unit->type()) + ") cannot move", id);
			return errors;
		}
		if (!_world->map().contains(target))
		{
			errors.add(
				"march.out_of_bounds", "target " + describe(target) + " is outside the map " + mapSize(*_world), id);
			return errors;
		}
		const Position startsFrom = movement->marches.empty() ? unit->position() : movement->marches.back();
		if (target == startsFrom)
		{
			errors.add(
				"march.same_position",
				unitName(id) + " will already be at " + describe(target) + " when this march starts",
				id);
			return errors;
		}

		_movement->addMarch(*unit, target);
		return {};
	}

	Result<void> SetupContext::finish()
	{
		Diagnostics errors;
		if (!_mapCommandSeen && !_missingMapReported)
		{
			errors.add("map.not_created", "scenario has no CREATE_MAP command");
			_missingMapReported = true;
		}
		return toResult(std::move(errors));
	}
}
