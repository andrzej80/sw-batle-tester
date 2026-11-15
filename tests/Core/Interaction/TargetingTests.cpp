#include "Core/Interaction/InteractionTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <type_traits>

using sw::core::ComponentStore;
using sw::core::DistanceRange;
using sw::core::Effect;
using sw::core::Intent;
using sw::core::ITargetRule;
using sw::core::TargetSpec;
using sw::core::Unit;
using namespace sw::test;

namespace
{
	// Сдвигает минимальную границу на +1 и записывает, что получило
	class ShiftingRule final : public sw::core::IComponent, public ITargetRule
	{
	public:
		explicit ShiftingRule(std::vector<DistanceRange>& seen) :
				_seen(seen)
		{}

		std::optional<DistanceRange> adjust(const TargetSpec&, DistanceRange range) const override
		{
			_seen.push_back(range);
			return range.shifted(+1, 0);
		}

	private:
		std::vector<DistanceRange>& _seen;
	};

	class RecordingRule final : public sw::core::IComponent, public ITargetRule
	{
	public:
		explicit RecordingRule(std::vector<DistanceRange>& seen) :
				_seen(seen)
		{}

		std::optional<DistanceRange> adjust(const TargetSpec&, DistanceRange range) const override
		{
			_seen.push_back(range);
			return range;
		}

	private:
		std::vector<DistanceRange>& _seen;
	};
}

// Автор действия обязан выбрать Intent и Effect
static_assert(!std::is_default_constructible_v<TargetSpec>);

TEST(Targeting_skips_source_dead_and_removed_units)
{
	Arena arena;
	const Unit& source = arena.spawn({.id = 1, .position = {5, 5}});
	arena.spawn({.id = 2, .position = {6, 5}});
	Unit& dead = arena.spawn({.id = 3, .position = {4, 5}});
	Unit& removed = arena.spawn({.id = 4, .position = {5, 4}});
	arena.spawn({.id = 5, .position = {5, 6}});
	kill(dead);
	removed.requestRemoval();

	const TargetSpec anyDistance{Intent::Hostile, Effect::Damage, {Melee}, {0, 10}};
	CHECK(idsOf(arena.targeting.select(source, anyDistance)) == (std::vector<uint32_t>{2, 5}));
}

TEST(Targeting_respects_distance)
{
	Arena arena;
	const Unit& hunter = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn({.id = 2, .position = {1, 0}});
	arena.spawn({.id = 3, .position = {2, 2}});
	arena.spawn({.id = 4, .position = {4, 0}});
	arena.spawn({.id = 5, .position = {5, 0}});

	CHECK(idsOf(arena.targeting.select(hunter, rangedAttack({2, 4}))) == (std::vector<uint32_t>{3, 4}));
	CHECK(idsOf(arena.targeting.select(hunter, meleeAttack())) == (std::vector<uint32_t>{2}));
}

TEST(Targeting_damage_and_heal_need_health_but_status_does_not)
{
	Arena arena;
	const Unit& source = arena.spawn({.id = 1, .position = {5, 5}});
	arena.spawn({.id = 2, .position = {6, 5}, .occupiesCell = false, .hp = std::nullopt});	// мина

	CHECK(arena.targeting.select(source, meleeAttack()).empty());
	CHECK(arena.targeting.select(source, touchHeal()).empty());
	CHECK(idsOf(arena.targeting.select(source, debuff({1, 1}))) == (std::vector<uint32_t>{2}));
}

TEST(Targeting_applies_action_filter)
{
	Arena arena;
	const Unit& healer = arena.spawn({.id = 1, .position = {5, 5}});
	Unit& wounded = arena.spawn({.id = 2, .position = {6, 5}});
	arena.spawn({.id = 3, .position = {4, 5}});
	wounded.get<sw::core::Health>().current = 3;

	const auto notFull = [](const Unit& unit)
	{
		return unit.get<sw::core::Health>().current < unit.get<sw::core::Health>().max;
	};
	CHECK(idsOf(arena.targeting.select(healer, touchHeal(), notFull)) == (std::vector<uint32_t>{2}));
}

// Правила цели задаёт цель: иммунитет Ворона к ближнему бою, стрелкам дистанция −1,
// лечение и баффы не затрагиваются
TEST(Targeting_raven_rule_limits_only_hostile_actions)
{
	Arena arena;
	const Unit& attacker = arena.spawn({.id = 1, .position = {5, 5}});
	const auto flying = [](ComponentStore& components)
	{
		components.add<FlyingRule>();
	};
	arena.spawn({.id = 2, .position = {6, 5}, .occupiesCell = false, .components = flying});  // дистанция 1
	arena.spawn({.id = 3, .position = {9, 5}, .occupiesCell = false, .components = flying});  // дистанция 4

	CHECK(arena.targeting.select(attacker, meleeAttack()).empty());
	// {2,4} для Ворона становится {1,3}: ближний — в досягаемости, дальний — уже нет
	CHECK(idsOf(arena.targeting.select(attacker, rangedAttack({2, 4}))) == (std::vector<uint32_t>{2}));
	// лечение касанием (тег melee, но Beneficial) правило не трогает
	CHECK(idsOf(arena.targeting.select(attacker, touchHeal())) == (std::vector<uint32_t>{2}));
}

TEST(Targeting_rules_are_chained_in_order_of_addition)
{
	Arena arena;
	std::vector<DistanceRange> seen;
	const Unit& source = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn(
		{.id = 2,
		 .position = {2, 0},
		 .components = [&](ComponentStore& components)
		 {
			 components.add<ShiftingRule>(seen);
			 components.add<RecordingRule>(seen);
		 }});

	// {2,4} → ShiftingRule → {3,4}: цель на дистанции 2 уже недосягаема
	CHECK(arena.targeting.select(source, rangedAttack({2, 4})).empty());
	CHECK(seen == (std::vector<DistanceRange>{{2, 4}, {3, 4}}));
}

TEST(Targeting_denying_rule_stops_the_chain)
{
	Arena arena;
	std::vector<DistanceRange> seen;
	const Unit& source = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn(
		{.id = 2,
		 .position = {1, 0},
		 .components = [&](ComponentStore& components)
		 {
			 components.add<UntargetableRule>();
			 components.add<RecordingRule>(seen);
		 }});

	CHECK(arena.targeting.select(source, meleeAttack()).empty());
	CHECK(arena.targeting.select(source, debuff({0, 5})).empty());
	CHECK(seen.empty());
}
