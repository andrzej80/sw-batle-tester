#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <Core/Units/ComponentRequirements.hpp>
#include <Core/Units/Unit.hpp>
#include <cstdint>
#include <functional>
#include <utility>

namespace sw::features
{
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
