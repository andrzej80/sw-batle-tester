#pragma once

#include "TargetSpec.hpp"

#include <functional>
#include <vector>

namespace sw::core
{
	class Unit;
	class UnitStore;

	class Targeting
	{
	public:
		using Filter = std::function<bool(const Unit&)>;

		explicit Targeting(const UnitStore& units);

		std::vector<const Unit*> select(const Unit& source, const TargetSpec& spec, const Filter& filter = {}) const;

		bool canTarget(const Unit& source, const Unit& target, const TargetSpec& spec) const;

	private:
		const UnitStore& _units;
	};
}
