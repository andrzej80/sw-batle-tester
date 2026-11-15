#include "Interaction.hpp"

#include "IHitModifier.hpp"
#include "Targeting.hpp"

#include <Core/Common/TypeName.hpp>
#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <Core/World/World.hpp>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

namespace sw::core
{
	namespace
	{
		std::string describe(const Hit& hit)
		{
			return "hit from unit " + std::to_string(hit.source.value) + " to unit " + std::to_string(hit.target.value);
		}

		void applyModifiers(Unit& target, Hit& hit)
		{
			std::vector<IHitModifier*> modifiers = target.components().hitModifiers();
			std::stable_sort(
				modifiers.begin(),
				modifiers.end(),
				[](const IHitModifier* a, const IHitModifier* b) { return a->priority() < b->priority(); });
			for (IHitModifier* modifier : modifiers)
			{
				modifier->modify(hit);
				if (hit.amount < 0)
				{
					throw std::logic_error(
						"Interaction: modifier " + typeName(*modifier) + " made amount negative (" + describe(hit)
						+ ")");
				}
			}
		}
	}

	Interaction::Interaction(World& world, const Targeting& targeting, const EventBus& events) :
			_world(world),
			_targeting(targeting),
			_events(events)
	{}

	void Interaction::apply(const Hit& incoming)
	{
		if (incoming.spec.effect == Effect::Status)
		{
			throw std::logic_error(
				"Interaction: apply() is for Damage and Heal, use applyStatus() (" + describe(incoming) + ")");
		}
		if (incoming.amount < 0)
		{
			throw std::logic_error("Interaction: negative amount (" + describe(incoming) + ")");
		}
		Unit* target = _world.units().find(incoming.target);
		if (target == nullptr || !isAlive(*target) || !target->has<Health>())
		{
			throw std::logic_error(
				"Interaction: target must be alive and have Health; select it through Targeting (" + describe(incoming)
				+ ")");
		}

		Hit hit = incoming;
		applyModifiers(*target, hit);

		Health& health = target->get<Health>();
		if (hit.spec.effect == Effect::Damage)
		{
			const bool wasAlive = health.current > 0;
			health.current -= hit.amount;
			_events.publish(
				events::UnitAttacked{hit.source, hit.target, hit.amount, std::max<int64_t>(health.current, 0)});
			if (wasAlive && health.current <= 0)
			{
				_events.publish(events::UnitDied{hit.target});
			}
		}
		else
		{
			health.current = std::min(health.max, health.current + hit.amount);
			_events.publish(events::UnitHealed{hit.source, hit.target, hit.amount, health.current});
		}
		_world.activity().markChange();
	}

	bool Interaction::applyStatus(
		const Unit& source, const Unit& target, const TargetSpec& spec, std::unique_ptr<Status> status)
	{
		if (spec.effect != Effect::Status)
		{
			throw std::logic_error("Interaction: applyStatus() requires a TargetSpec with Effect::Status");
		}
		if (!status)
		{
			throw std::logic_error("Interaction: null status");
		}
		if (!_targeting.canTarget(source, target, spec))
		{
			return false;
		}

		Unit* mutableTarget = _world.units().find(target.id());
		ComponentStore& components = mutableTarget->components();
		const Status& incoming = *status;
		const std::type_index type(typeid(incoming));
		if (auto* existing = dynamic_cast<Status*>(components.find(type)))
		{
			if (existing->onReapplied(incoming) == Reapply::Keep)
			{
				_world.activity().markChange();
				return true;
			}
			components.remove(type);
		}
		components.add(type, std::move(status));
		_world.activity().markChange();
		return true;
	}
}
