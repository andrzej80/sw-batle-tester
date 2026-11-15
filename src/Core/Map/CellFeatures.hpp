#pragma once

#include <cstdint>

namespace sw::core
{
	class TurnContext;
	class Unit;

	// Свойство клетки, меняющее стоимость входа в неё: болото, ускорение
	// (ARCHITECTURE.md, п. 4.8). Хук получает юнита, поэтому летающих можно отсекать
	// по тегу flying.
	//
	// Контракт (п. 4.13): чистая функция; результат ≥ 1. Модификаторы клетки применяются
	// по очереди в порядке добавления, каждый получает стоимость после предыдущего.
	class IMoveCostModifier
	{
	public:
		IMoveCostModifier() = default;
		virtual ~IMoveCostModifier() = default;

		IMoveCostModifier(const IMoveCostModifier&) = delete;
		IMoveCostModifier& operator=(const IMoveCostModifier&) = delete;
		IMoveCostModifier(IMoveCostModifier&&) = delete;
		IMoveCostModifier& operator=(IMoveCostModifier&&) = delete;

		virtual int64_t moveCost(const Unit& unit, int64_t cost) const = 0;
	};

	// Свойство клетки, срабатывающее при входе юнита: капкан.
	// Контракт (п. 4.13): меняет мир только через TurnContext.
	class IEnterTrigger
	{
	public:
		IEnterTrigger() = default;
		virtual ~IEnterTrigger() = default;

		IEnterTrigger(const IEnterTrigger&) = delete;
		IEnterTrigger& operator=(const IEnterTrigger&) = delete;
		IEnterTrigger(IEnterTrigger&&) = delete;
		IEnterTrigger& operator=(IEnterTrigger&&) = delete;

		virtual void onEnter(const Unit& unit, TurnContext& context) = 0;
	};
}
