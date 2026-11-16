#include "Targeting.hpp"

#include "ITargetRule.hpp"

#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <Core/World/SpatialQuery.hpp>
#include <Core/World/UnitStore.hpp>
#include <optional>

namespace sw::core
{
	Targeting::Targeting(const UnitStore& units) :
			_units(units)
	{}

	std::vector<const Unit*> Targeting::select(const Unit& source, const TargetSpec& spec, const Filter& filter) const
	{
		std::vector<const Unit*> targets;
		for (const Unit* candidate : _units.ordered())
		{
			if (canTarget(source, *candidate, spec) && (!filter || filter(*candidate)))
			{
				targets.push_back(candidate);
			}
		}
		return targets;
	}

	bool Targeting::canTarget(const Unit& source, const Unit& target, const TargetSpec& spec) const
	{
		if (target.id() == source.id() || !isAlive(target))
		{
			return false;
		}
		const bool changesHealth = spec.effect == Effect::Damage || spec.effect == Effect::Heal;
		if (changesHealth && !target.has<Health>())
		{
			return false;
		}

		std::optional<DistanceRange> range = spec.range;
		for (const ITargetRule* rule : target.components().targetRules())
		{
			range = rule->adjust(spec, *range);
			if (!range)
			{
				return false;
			}
		}
		return range->contains(chebyshev(source.position(), target.position()));
	}
}
