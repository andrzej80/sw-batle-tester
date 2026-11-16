#pragma once

#include "Core/World/WorldTestHelpers.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Interaction/IHitModifier.hpp>
#include <Core/Interaction/ITargetRule.hpp>
#include <Core/Interaction/Interaction.hpp>
#include <Core/Interaction/Targeting.hpp>
#include <Core/World/World.hpp>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace sw::test
{
	inline const core::Tag Melee{"melee"};
	inline const core::Tag Ranged{"ranged"};
	inline const core::Tag Area{"area"};
	inline const core::Tag Physical{"physical"};
	inline const core::Tag HealTag{"heal"};

	inline core::TargetSpec meleeAttack()
	{
		return {core::Intent::Hostile, core::Effect::Damage, {Melee, Physical}, {1, 1}};
	}

	inline core::TargetSpec rangedAttack(core::DistanceRange range)
	{
		return {core::Intent::Hostile, core::Effect::Damage, {Ranged, Physical}, range};
	}

	inline core::TargetSpec touchHeal()
	{
		return {core::Intent::Beneficial, core::Effect::Heal, {Melee, HealTag}, {0, 1}};
	}

	inline core::TargetSpec debuff(core::DistanceRange range)
	{
		return {core::Intent::Hostile, core::Effect::Status, {Melee}, range};
	}

	struct FlyingRule final : core::IComponent, core::ITargetRule
	{
		std::optional<core::DistanceRange> adjust(const core::TargetSpec& in, core::DistanceRange range) const override
		{
			if (in.intent != core::Intent::Hostile)
			{
				return range;
			}
			if (in.tags.has(Melee))
			{
				return std::nullopt;
			}
			if (in.tags.has(Ranged))
			{
				return range.shifted(-1, -1);
			}
			return range;
		}
	};

	struct UntargetableRule final : core::IComponent, core::ITargetRule
	{
		std::optional<core::DistanceRange> adjust(const core::TargetSpec&, core::DistanceRange) const override
		{
			return std::nullopt;
		}
	};

	template <int Id>
	class LoggingModifier final : public core::IComponent, public core::IHitModifier
	{
	public:
		LoggingModifier(std::vector<std::string>& log, std::string name, int priority) :
				_log(log),
				_name(std::move(name)),
				_priority(priority)
		{}

		int priority() const override
		{
			return _priority;
		}

		void modify(core::Hit&) override
		{
			_log.push_back(_name);
		}

	private:
		std::vector<std::string>& _log;
		std::string _name;
		int _priority;
	};

	class Shield final : public core::IComponent, public core::IHitModifier
	{
	public:
		explicit Shield(int64_t capacity) :
				_left(capacity)
		{}

		int priority() const override
		{
			return 0;
		}

		void modify(core::Hit& hit) override
		{
			if (hit.spec.effect != core::Effect::Damage)
			{
				return;
			}
			const int64_t absorbed = std::min(_left, hit.amount);
			_left -= absorbed;
			hit.amount -= absorbed;
		}

	private:
		int64_t _left;
	};

	struct HealBan final : core::IComponent, core::IHitModifier
	{
		int priority() const override
		{
			return 0;
		}

		void modify(core::Hit& hit) override
		{
			if (hit.spec.effect == core::Effect::Heal)
			{
				hit.amount = 0;
			}
		}
	};

	struct NegativeModifier final : core::IComponent, core::IHitModifier
	{
		int priority() const override
		{
			return 0;
		}

		void modify(core::Hit& hit) override
		{
			hit.amount = -1;
		}
	};

	struct Burning final : core::Status
	{
		explicit Burning(core::Tick expiresAt) :
				Status(expiresAt)
		{}
	};

	class StackingPoison final : public core::Status
	{
	public:
		explicit StackingPoison(core::Tick expiresAt) :
				Status(expiresAt)
		{}

		core::Reapply onReapplied(const core::Status& incoming) override
		{
			++stacks;
			setExpiresAt(incoming.expiresAt());
			return core::Reapply::Keep;
		}

		int stacks = 1;
	};

	class ShieldStatus final : public core::Status, public core::IHitModifier
	{
	public:
		ShieldStatus() :
				Status(std::nullopt)
		{}

		int priority() const override
		{
			return 0;
		}

		void modify(core::Hit& hit) override
		{
			hit.amount = 0;
		}
	};

	struct Arena
	{
		Arena()
		{
			bus.subscribe<core::events::UnitAttacked>(
				[this](const core::events::UnitAttacked& e)
				{
					log.push_back(
						"attacked " + std::to_string(e.attackerId.value) + "->" + std::to_string(e.targetId.value)
						+ " damage=" + std::to_string(e.damage) + " hp=" + std::to_string(e.targetHp));
				});
			bus.subscribe<core::events::UnitDied>([this](const core::events::UnitDied& e)
												  { log.push_back("died " + std::to_string(e.unitId.value)); });
			bus.subscribe<core::events::UnitHealed>(
				[this](const core::events::UnitHealed& e)
				{
					log.push_back(
						"healed " + std::to_string(e.healerId.value) + "->" + std::to_string(e.targetId.value)
						+ " amount=" + std::to_string(e.amount) + " hp=" + std::to_string(e.targetHp));
				});
		}

		Arena(const Arena&) = delete;
		Arena& operator=(const Arena&) = delete;

		core::Unit& spawn(const TestUnit& description)
		{
			return world.place(makeUnit(description));
		}

		core::World world{10, 10};
		core::EventBus bus;
		core::Targeting targeting{world.units()};
		core::Interaction interaction{world, targeting, bus};
		std::vector<std::string> log;
	};

	inline std::vector<uint32_t> idsOf(const std::vector<const core::Unit*>& units)
	{
		std::vector<uint32_t> ids;
		for (const core::Unit* unit : units)
		{
			ids.push_back(unit->id().value);
		}
		return ids;
	}
}
