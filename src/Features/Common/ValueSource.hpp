#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <Core/Units/ComponentRequirements.hpp>
#include <Core/Units/Unit.hpp>
#include <cstdint>
#include <functional>
#include <utility>

// Откуда действие берёт число: константа или характеристика юнита (SPECIFICATION.md,
// п. 9.1). Значение читается в момент выполнения действия, а не при сборке: эффект,
// меняющий характеристику (бафф дальности, проклятие силы), сразу влияет на действие.
//
//   MeleeAttack(stat<Strength>)
//   RangedAttack(stat<Agility>, distance(2, stat<Range>))
//   RangedAttack(stat<Power>, distance(2, 5))
namespace sw::features
{
	// Характеристика T юнита. stat<T> — единственная точка чтения характеристик: будущие
	// модификаторы характеристик добавляются здесь, без правок Core и действий.
	template <class T>
	struct Stat
	{
		int64_t operator()(const core::Unit& unit) const
		{
			return unit.get<T>().value;
		}
	};

	template <class T>
	inline constexpr Stat<T> stat{};

	// Число для действия: константа или характеристика. Объявляет требования к компонентам,
	// чтобы юнит без нужной характеристики не собрался.
	class ValueSource
	{
	public:
		ValueSource(int64_t constant) :
				_read([constant](const core::Unit&) { return constant; })
		{}

		ValueSource(int constant) :
				ValueSource(static_cast<int64_t>(constant))
		{}

		template <class T>
		ValueSource(Stat<T> source) :
				_read(source),
				_declare([](core::ComponentRequirements& requirements) { requirements.require<T>(); })
		{}

		int64_t read(const core::Unit& unit) const
		{
			return _read(unit);
		}

		void declare(core::ComponentRequirements& requirements) const
		{
			if (_declare)
			{
				_declare(requirements);
			}
		}

	private:
		std::function<int64_t(const core::Unit&)> _read;
		std::function<void(core::ComponentRequirements&)> _declare;
	};

	// Дистанция действия; каждая граница — число или характеристика. min > max (например,
	// дальность опустили ниже 2) — пустой диапазон: целей нет.
	class Distance
	{
	public:
		Distance(ValueSource min, ValueSource max) :
				_min(std::move(min)),
				_max(std::move(max))
		{}

		core::DistanceRange resolve(const core::Unit& unit) const
		{
			return {_min.read(unit), _max.read(unit)};
		}

		void declare(core::ComponentRequirements& requirements) const
		{
			_min.declare(requirements);
			_max.declare(requirements);
		}

	private:
		ValueSource _min;
		ValueSource _max;
	};

	inline Distance distance(ValueSource min, ValueSource max)
	{
		return {std::move(min), std::move(max)};
	}
}
