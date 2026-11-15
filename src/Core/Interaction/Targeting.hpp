#pragma once

#include "TargetSpec.hpp"

#include <functional>
#include <vector>

namespace sw::core
{
	class Unit;
	class UnitStore;

	// Отбор целей с правилами ЦЕЛИ (SPECIFICATION.md, п. 5.2). Выбор случайной из
	// кандидатов — на стороне действия.
	class Targeting
	{
	public:
		using Filter = std::function<bool(const Unit&)>;

		explicit Targeting(const UnitStore& units);

		// Подходящие цели в порядке создания. filter — дополнительное условие действия
		// (например, «HP не полное» у лечения).
		std::vector<const Unit*> select(const Unit& source, const TargetSpec& spec, const Filter& filter = {}) const;

		// Можно ли выбрать target этим воздействием:
		// 1. target — не источник, не мёртв и не помечен на удаление;
		// 2. для Damage и Heal у target есть Health;
		// 3. правила цели по очереди меняют дистанцию, nullopt — запрет;
		// 4. расстояние Чебышёва попадает в итоговую дистанцию.
		bool canTarget(const Unit& source, const Unit& target, const TargetSpec& spec) const;

	private:
		const UnitStore& _units;
	};
}
