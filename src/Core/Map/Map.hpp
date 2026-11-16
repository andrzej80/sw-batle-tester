#pragma once

#include "CellFeatures.hpp"

#include <Core/Common/Types.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
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

		template <class F>
		void addCellFeature(Position cell, std::shared_ptr<F> feature)
		{
			constexpr bool isCost = std::is_base_of_v<IMoveCostModifier, F>;
			constexpr bool isTrigger = std::is_base_of_v<IEnterTrigger, F>;
			static_assert(isCost || isTrigger, "cell feature must implement IMoveCostModifier and/or IEnterTrigger");

			const IMoveCostModifier* cost = nullptr;
			IEnterTrigger* trigger = nullptr;
			if constexpr (isCost)
			{
				cost = feature.get();
			}
			if constexpr (isTrigger)
			{
				trigger = feature.get();
			}
			addCellFeature(cell, std::shared_ptr<const void>(std::move(feature)), cost, trigger);
		}

		int64_t moveCost(Position cell, const Unit& unit) const;

		void triggerEnter(Position cell, const Unit& unit, TurnContext& context) const;

	private:
		struct CellFeatureList
		{
			std::vector<std::shared_ptr<const void>> owners;
			std::vector<const IMoveCostModifier*> costModifiers;
			std::vector<IEnterTrigger*> enterTriggers;
		};

		void requireInside(Position cell, const char* operation) const;
		void addCellFeature(
			Position cell, std::shared_ptr<const void> owner, const IMoveCostModifier* cost, IEnterTrigger* trigger);

		int64_t _width;
		int64_t _height;
		std::unordered_map<Position, UnitId> _occupants;
		std::unordered_map<Position, CellFeatureList> _features;
	};
}
