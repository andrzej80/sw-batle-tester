#include "Healer.hpp"

#include <Core/Actions/IAction.hpp>
#include <Core/Actions/MarchStep.hpp>
#include <Core/Common/Tag.hpp>
#include <Core/Interaction/Interaction.hpp>
#include <Core/Interaction/Targeting.hpp>
#include <Core/Simulation/TurnContext.hpp>
#include <Core/Units/CoreComponents.hpp>
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
		const core::Tag HealTag{"heal"};

		bool isWounded(const core::Unit& unit)
		{
			const auto& health = unit.get<core::Health>();
			return health.current < health.max;
		}

		class MinorHeal final : public core::IAction
		{
		public:
			MinorHeal(ValueSource amount, Distance reach) :
					_amount(std::move(amount)),
					_reach(std::move(reach))
			{}

			bool tryExecute(const core::Unit& self, core::TurnContext& context) override
			{
				const core::TargetSpec spec = heal({HealTag}, _reach.resolve(self));
				const std::vector<const core::Unit*> targets = context.targeting().select(self, spec, isWounded);
				if (targets.empty())
				{
					return false;
				}
				const core::Unit* target = targets[context.random().index(targets.size())];
				context.effects().apply(
					core::Hit{spec, self.id(), target->id(), std::max<int64_t>(_amount.read(self), 0)});
				return true;
			}

			void requirements(core::ComponentRequirements& requirements) const override
			{
				_amount.declare(requirements);
				_reach.declare(requirements);
			}

		private:
			ValueSource _amount;
			Distance _reach;
		};
	}

	core::SpawnRequest makeHealer(const HealerSpec& spec)
	{
		UnitBuilder builder(spec.id, "Healer", spec.position);

		core::Diagnostics errors;
		requirePositive(errors, "hp", spec.hp);
		requirePositive(errors, "spirit", spec.spirit);
		if (!errors.empty())
		{
			return builder.reject(errors);
		}

		return builder.with<core::Health>(spec.hp)
			.with<Spirit>(spec.spirit)
			.with<core::Movement>(1)
			.action<MinorHeal>(stat<Spirit>, distance(0, 2))
			.action<core::MarchStep>()
			.spawnRequest();
	}
}
