#include "Attacks.hpp"

#include "CommonTags.hpp"
#include "TargetSpecs.hpp"

#include <Core/Interaction/Interaction.hpp>
#include <Core/Interaction/Targeting.hpp>
#include <Core/Simulation/TurnContext.hpp>
#include <Core/World/SpatialQuery.hpp>
#include <algorithm>
#include <utility>
#include <vector>

namespace sw::features
{
	namespace
	{
		bool hitRandomTarget(
			const core::Unit& self, core::TurnContext& context, const core::TargetSpec& spec, int64_t damage)
		{
			const std::vector<const core::Unit*> targets = context.targeting().select(self, spec);
			if (targets.empty())
			{
				return false;
			}
			const core::Unit* target = targets[context.random().index(targets.size())];
			context.effects().apply(core::Hit{spec, self.id(), target->id(), std::max<int64_t>(damage, 0)});
			return true;
		}
	}

	MeleeAttack::MeleeAttack(ValueSource damage) :
			_damage(std::move(damage))
	{}

	bool MeleeAttack::tryExecute(const core::Unit& self, core::TurnContext& context)
	{
		return hitRandomTarget(self, context, attack({tags::Melee, tags::Physical}, {1, 1}), _damage.read(self));
	}

	void MeleeAttack::requirements(core::ComponentRequirements& requirements) const
	{
		_damage.declare(requirements);
	}

	RangedAttack::RangedAttack(ValueSource damage, Distance distance, ShotCondition condition) :
			_damage(std::move(damage)),
			_distance(std::move(distance)),
			_condition(std::move(condition))
	{}

	bool RangedAttack::tryExecute(const core::Unit& self, core::TurnContext& context)
	{
		if (_condition && !_condition(self, context))
		{
			return false;
		}
		const core::TargetSpec spec = attack({tags::Ranged, tags::Physical}, _distance.resolve(self));
		return hitRandomTarget(self, context, spec, _damage.read(self));
	}

	void RangedAttack::requirements(core::ComponentRequirements& requirements) const
	{
		_damage.declare(requirements);
		_distance.declare(requirements);
	}

	bool RequireNoAdjacentOccupying::operator()(const core::Unit& self, const core::TurnContext& context) const
	{
		const std::vector<const core::Unit*> neighbours = context.query().neighbours(self.position());
		return std::none_of(
			neighbours.begin(), neighbours.end(), [](const core::Unit* unit) { return unit->occupiesCell(); });
	}
}
