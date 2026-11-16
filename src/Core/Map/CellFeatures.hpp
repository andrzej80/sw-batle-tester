#pragma once

#include <cstdint>

namespace sw::core
{
	class TurnContext;
	class Unit;

	// Контракт: чистая функция, результат не меньше 1.
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
