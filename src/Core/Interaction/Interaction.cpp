#include "Interaction.hpp"

#include "Targeting.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <Core/World/World.hpp>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <typeindex>

namespace sw::core
{
	namespace
	{
		std::string describe(const Hit& hit)
		{
			return "hit from unit " + std::to_string(hit.source.value) + " to unit " + std::to_string(hit.target.value);
		}
	}

	Interaction::Interaction(World& world, const Targeting& targeting, const EventBus& events) :
			_world(world),
			_targeting(targeting),
			_events(events)
	{}

	void Interaction::apply(const Hit& hit)
	{
		if (hit.spec.effect == Effect::Status)
		{
			throw std::logic_error(
				"Interaction: apply() is for Damage and Heal, use applyStatus() (" + describe(hit) + ")");
		}
		if (hit.amount < 0)
		{
			throw std::logic_error("Interaction: negative amount (" + describe(hit) + ")");
		}
		const Unit* source = _world.units().find(hit.source);
		Unit* target = _world.units().find(hit.target);
		if (source == nullptr || target == nullptr || !_targeting.canTarget(*source, *target, hit.spec))
		{
			throw std::logic_error(
				"Interaction: target cannot be hit by this spec; select it through Targeting (" + describe(hit) + ")");
		}

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

		ComponentStore& components = _world.units().mutableOf(target).components();
		const Status& incoming = *status;
		const std::type_index type(typeid(incoming));
		components.remove(type);
		components.add(type, std::move(status));
		_world.activity().markChange();
		return true;
	}
}
