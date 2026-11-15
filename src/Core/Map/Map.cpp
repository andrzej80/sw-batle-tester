#include "Map.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace sw::core
{
	namespace
	{
		int64_t requirePositiveSize(int64_t size, const char* what)
		{
			if (size <= 0)
			{
				throw std::logic_error(std::string("Map: ") + what + " must be positive, got " + std::to_string(size));
			}
			return size;
		}

		std::string describe(Position cell)
		{
			std::ostringstream stream;
			stream << cell;
			return stream.str();
		}
	}

	Map::Map(int64_t width, int64_t height) :
			_width(requirePositiveSize(width, "width")),
			_height(requirePositiveSize(height, "height"))
	{}

	bool Map::contains(Position cell) const
	{
		return cell.x >= 0 && cell.x < _width && cell.y >= 0 && cell.y < _height;
	}

	std::optional<UnitId> Map::occupant(Position cell) const
	{
		const auto found = _occupants.find(cell);
		if (found == _occupants.end())
		{
			return std::nullopt;
		}
		return found->second;
	}

	void Map::occupy(Position cell, UnitId unit)
	{
		requireInside(cell, "occupy");
		const auto [existing, inserted] = _occupants.emplace(cell, unit);
		if (!inserted)
		{
			throw std::logic_error(
				"Map: cell " + describe(cell) + " is already occupied by unit "
				+ std::to_string(existing->second.value));
		}
	}

	void Map::release(Position cell, UnitId unit)
	{
		const auto found = _occupants.find(cell);
		if (found == _occupants.end() || found->second != unit)
		{
			throw std::logic_error(
				"Map: unit " + std::to_string(unit.value) + " does not occupy cell " + describe(cell));
		}
		_occupants.erase(found);
	}

	std::vector<Position> Map::neighbourCells(Position cell) const
	{
		std::vector<Position> cells;
		for (int64_t dy = -1; dy <= 1; ++dy)
		{
			for (int64_t dx = -1; dx <= 1; ++dx)
			{
				const Position neighbour{cell.x + dx, cell.y + dy};
				if ((dx != 0 || dy != 0) && contains(neighbour))
				{
					cells.push_back(neighbour);
				}
			}
		}
		return cells;
	}

	void Map::addCellFeature(
		Position cell, std::shared_ptr<const void> owner, const IMoveCostModifier* cost, IEnterTrigger* trigger)
	{
		requireInside(cell, "addCellFeature");
		CellFeatureList& features = _features[cell];
		features.owners.push_back(std::move(owner));
		if (cost != nullptr)
		{
			features.costModifiers.push_back(cost);
		}
		if (trigger != nullptr)
		{
			features.enterTriggers.push_back(trigger);
		}
	}

	int64_t Map::moveCost(Position cell, const Unit& unit) const
	{
		int64_t cost = 1;
		const auto found = _features.find(cell);
		if (found == _features.end())
		{
			return cost;
		}
		for (const IMoveCostModifier* modifier : found->second.costModifiers)
		{
			cost = modifier->moveCost(unit, cost);
			if (cost < 1)
			{
				throw std::logic_error(
					"Map: move cost of cell " + describe(cell) + " must be at least 1, got " + std::to_string(cost));
			}
		}
		return cost;
	}

	void Map::triggerEnter(Position cell, const Unit& unit, TurnContext& context) const
	{
		const auto found = _features.find(cell);
		if (found == _features.end())
		{
			return;
		}
		for (IEnterTrigger* trigger : found->second.enterTriggers)
		{
			trigger->onEnter(unit, context);
		}
	}

	void Map::requireInside(Position cell, const char* operation) const
	{
		if (!contains(cell))
		{
			throw std::logic_error(
				std::string("Map: ") + operation + ": cell " + describe(cell) + " is outside the map "
				+ std::to_string(_width) + "x" + std::to_string(_height));
		}
	}
}
