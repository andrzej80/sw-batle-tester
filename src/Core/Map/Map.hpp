#pragma once

#include <Core/Common/Types.hpp>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace sw::core
{
	// Хранение разреженное: карта может быть до 4294967295 × 4294967295, плотная сетка
	// такого размера в память не поместится.
	class Map
	{
	public:
		Map(int64_t width, int64_t height);

		int64_t width() const
		{
			return _width;
		}

		int64_t height() const
		{
			return _height;
		}

		bool contains(Position cell) const;

		std::optional<UnitId> occupant(Position cell) const;

		bool isOccupied(Position cell) const
		{
			return _occupants.contains(cell);
		}

		void occupy(Position cell, UnitId unit);
		void release(Position cell, UnitId unit);

		std::vector<Position> neighbourCells(Position cell) const;

	private:
		void requireInside(Position cell, const char* operation) const;

		int64_t _width;
		int64_t _height;
		std::unordered_map<Position, UnitId> _occupants;
	};
}
