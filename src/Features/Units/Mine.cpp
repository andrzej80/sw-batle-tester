#include "Mine.hpp"

#include <Core/Actions/IAction.hpp>
#include <Core/Common/DistanceRange.hpp>
#include <Core/Common/Tag.hpp>
#include <Core/Interaction/Interaction.hpp>
#include <Core/Interaction/Targeting.hpp>
#include <Core/Simulation/TurnContext.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/World/SpatialQuery.hpp>
#include <Features/Common/CommonTags.hpp>
#include <Features/Common/Stats.hpp>
#include <Features/Common/TargetSpecs.hpp>
#include <Features/Common/UnitBuilder.hpp>
#include <Features/Common/ValueSource.hpp>
#include <algorithm>
#include <utility>
#include <vector>

namespace sw::features
{
	namespace
	{
		const core::Tag Explosion{"explosion"};

		constexpr int64_t TriggerRadius = 2;
		constexpr int64_t BlastRadius = 3;

		void explode(core::TurnContext& context, core::UnitId mineId, const ValueSource& power)
		{
			const core::Unit* mine = context.query().find(mineId);
			if (mine == nullptr)
			{
				return;
			}
			context.publish(MineExploded{mineId});

			const core::TargetSpec spec = attack({tags::Area, Explosion}, {0, BlastRadius});
			const int64_t damage = std::max<int64_t>(power.read(*mine), 0);
			for (const core::Unit* target : context.targeting().select(*mine, spec))
			{
				context.effects().apply(core::Hit{spec, mineId, target->id(), damage});
			}
			context.requestRemoval(mineId);
		}

		class MineFuse final : public core::IAction
		{
		public:
			explicit MineFuse(ValueSource power) :
					_power(std::move(power))
			{}

			bool tryExecute(const core::Unit& self, core::TurnContext& context) override
			{
				if (_armed || !somebodyNear(self, context))
				{
					return false;
				}
				_armed = true;
				context.schedule(
					context.tick() + 1,
					self.id(),
					[mineId = self.id(), power = _power](core::TurnContext& later) { explode(later, mineId, power); });
				return true;
			}

			void requirements(core::ComponentRequirements& requirements) const override
			{
				_power.declare(requirements);
			}

		private:
			static bool somebodyNear(const core::Unit& self, const core::TurnContext& context)
			{
				const std::vector<const core::Unit*> nearby
					= context.query().unitsInRange(self.position(), {0, TriggerRadius});
				return std::any_of(
					nearby.begin(),
					nearby.end(),
					[&self](const core::Unit* unit) { return unit->id() != self.id() && unit->has<core::Health>(); });
			}

			ValueSource _power;
			bool _armed = false;
		};
	}

	core::SpawnRequest makeMine(const MineSpec& spec)
	{
		UnitBuilder builder(spec.id, "Mine", spec.position);
		builder.occupiesCell(false);

		core::Diagnostics errors;
		requirePositive(errors, "power", spec.power);
		if (!errors.empty())
		{
			return builder.reject(errors);
		}

		return builder.with<Power>(spec.power).action<MineFuse>(stat<Power>).spawnRequest();
	}
}
