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
	// Карта W×H (SPECIFICATION.md, п. 4.1): границы, занятость клеток, свойства клеток.
	//
	// Хранение разреженное: формат допускает карту до 4294967295 × 4294967295, плотная
	// сетка такого размера в память не поместится. Память — только под занятые клетки и
	// клетки со свойствами.
	//
	// Занятость хранят только юниты с occupiesCell. Мёртвый держит клетку до Cleanup.
	// Нарушения (клетка вне карты, уже занята, освобождает не тот юнит) — std::logic_error:
	// ошибки ввода ловит SetupContext раньше, здесь это баг.
	class Map
	{
	public:
		Map(int64_t width, int64_t height);	 // ≤ 0 — std::logic_error

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

		// Соседние клетки (расстояние 1) внутри карты. Считаются в int64_t, поэтому у
		// клетки (0,0) нет соседа (-1,0) — он отсекается, а не превращается в (4294967295,0).
		// Порядок постоянный: по строкам сверху вниз, в строке слева направо.
		std::vector<Position> neighbourCells(Position cell) const;

		// Свойство клетки реализует IMoveCostModifier и/или IEnterTrigger и раскладывается
		// по их спискам. Клетка вне карты — std::logic_error.
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

		// Стоимость входа юнита в клетку: 1, пропущенная через модификаторы клетки.
		// Результат < 1 — std::logic_error (нарушен контракт модификатора).
		int64_t moveCost(Position cell, const Unit& unit) const;

		// Вызывает триггеры клетки в порядке добавления
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
