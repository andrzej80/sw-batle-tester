#include "Core/Interaction/InteractionTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <memory>
#include <stdexcept>

using sw::core::ComponentStore;
using sw::core::Health;
using sw::core::Hit;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::test;

namespace
{
	Hit damage(uint32_t source, uint32_t target, int64_t amount)
	{
		return {meleeAttack(), UnitId{source}, UnitId{target}, amount};
	}

	Hit healing(uint32_t source, uint32_t target, int64_t amount)
	{
		return {touchHeal(), UnitId{source}, UnitId{target}, amount};
	}
}

TEST(Interaction_damage_reduces_hp_and_publishes_attack)
{
	Arena arena;
	arena.spawn({.id = 2, .position = {7, 0}});
	Unit& target = arena.spawn({.id = 1, .position = {6, 0}, .hp = 10});

	arena.interaction.apply(damage(2, 1, 4));

	CHECK_EQ(target.get<Health>().current, int64_t{6});
	CHECK(arena.log == (std::vector<std::string>{"attacked 2->1 damage=4 hp=6"}));
}

TEST(Interaction_lethal_damage_publishes_death_right_after_attack)
{
	Arena arena;
	arena.spawn({.id = 2, .position = {7, 0}});
	Unit& target = arena.spawn({.id = 1, .position = {6, 0}, .hp = 5});

	arena.interaction.apply(damage(2, 1, 7));

	CHECK(target.isDead());
	CHECK_EQ(target.get<Health>().current, int64_t{-2});
	CHECK(arena.log == (std::vector<std::string>{"attacked 2->1 damage=7 hp=0", "died 1"}));
}

TEST(Interaction_dead_target_cannot_be_hit_again)
{
	Arena arena;
	arena.spawn({.id = 2, .position = {7, 0}});
	arena.spawn({.id = 1, .position = {6, 0}, .hp = 5});
	arena.interaction.apply(damage(2, 1, 5));

	CHECK_THROWS(arena.interaction.apply(damage(2, 1, 5)), std::logic_error);
	CHECK_EQ(arena.log.size(), size_t{2});
}

TEST(Interaction_heal_is_capped_by_max_hp)
{
	Arena arena;
	arena.spawn({.id = 2, .position = {5, 5}});
	Unit& target = arena.spawn({.id = 1, .position = {6, 5}, .hp = 10});
	target.get<Health>().current = 7;

	arena.interaction.apply(healing(2, 1, 5));

	CHECK_EQ(target.get<Health>().current, int64_t{10});
	CHECK(arena.log == (std::vector<std::string>{"healed 2->1 amount=5 hp=10"}));
}

TEST(Interaction_heal_ban_turns_heal_into_zero_but_still_publishes_it)
{
	Arena arena;
	arena.spawn({.id = 2, .position = {5, 5}});
	Unit& target = arena.spawn(
		{.id = 1,
		 .position = {6, 5},
		 .components = [](ComponentStore& components)
		 {
			 components.add<HealBan>();
		 }});
	target.get<Health>().current = 4;

	arena.interaction.apply(healing(2, 1, 3));
	arena.interaction.apply(damage(2, 1, 1));

	CHECK_EQ(target.get<Health>().current, int64_t{3});
	CHECK(arena.log == (std::vector<std::string>{"healed 2->1 amount=0 hp=4", "attacked 2->1 damage=1 hp=3"}));
}

TEST(Interaction_modifiers_run_by_priority_then_by_order_of_addition)
{
	Arena arena;
	std::vector<std::string> calls;
	arena.spawn({.id = 2, .position = {5, 5}});
	arena.spawn(
		{.id = 1,
		 .position = {6, 5},
		 .components = [&](ComponentStore& components)
		 {
			 components.add<LoggingModifier<1>>(calls, "late", 2);
			 components.add<ShieldStatus>();
		 }});

	arena.interaction.apply(damage(2, 1, 3));
	CHECK(calls == (std::vector<std::string>{"late"}));
	CHECK(arena.log == (std::vector<std::string>{"attacked 2->1 damage=0 hp=10"}));
}

TEST(Interaction_modifiers_with_equal_priority_keep_order_of_addition)
{
	Arena arena;
	std::vector<std::string> calls;
	arena.spawn({.id = 2, .position = {5, 5}});
	arena.spawn(
		{.id = 1,
		 .position = {6, 5},
		 .components = [&](ComponentStore& components)
		 {
			 components.add<LoggingModifier<1>>(calls, "priority-2", 2);
			 components.add<LoggingModifier<2>>(calls, "priority-1-added-first", 1);
			 components.add<StackingPoison>(sw::core::Tick{100});
			 components.add<LoggingModifier<3>>(calls, "priority-1-added-second", 1);
		 }});

	arena.interaction.apply(damage(2, 1, 1));
	CHECK(calls == (std::vector<std::string>{"priority-1-added-first", "priority-1-added-second", "priority-2"}));
}

TEST(Interaction_shield_keeps_its_state_between_hits)
{
	Arena arena;
	arena.spawn({.id = 2, .position = {5, 5}});
	Unit& target = arena.spawn(
		{.id = 1,
		 .position = {6, 5},
		 .components = [](ComponentStore& components)
		 {
			 components.add<Shield>(3);
		 }});

	arena.interaction.apply(damage(2, 1, 2));
	arena.interaction.apply(damage(2, 1, 2));

	CHECK_EQ(target.get<Health>().current, int64_t{9});
	CHECK(arena.log == (std::vector<std::string>{"attacked 2->1 damage=0 hp=10", "attacked 2->1 damage=1 hp=9"}));
}

TEST(Interaction_rejects_contract_violations)
{
	Arena arena;
	arena.spawn({.id = 2, .position = {5, 5}});
	arena.spawn({.id = 1, .position = {6, 5}});
	arena.spawn({.id = 3, .position = {4, 5}, .hp = std::nullopt});
	arena.spawn(
		{.id = 4,
		 .position = {5, 4},
		 .components = [](ComponentStore& components)
		 {
			 components.add<NegativeModifier>();
		 }});

	CHECK_THROWS(arena.interaction.apply(damage(2, 1, -1)), std::logic_error);
	CHECK_THROWS(arena.interaction.apply(damage(2, 3, 1)), std::logic_error);
	CHECK_THROWS(arena.interaction.apply(damage(2, 99, 1)), std::logic_error);
	CHECK_THROWS(arena.interaction.apply(damage(2, 4, 1)), std::logic_error);
	const Hit status{debuff({1, 1}), UnitId{2}, UnitId{1}, 1};
	CHECK_THROWS(arena.interaction.apply(status), std::logic_error);
	CHECK(arena.log.empty());
}

TEST(Interaction_hit_built_past_targeting_cannot_bypass_its_rules)
{
	Arena arena;
	arena.spawn({.id = 2, .position = {5, 5}});
	arena.spawn(
		{.id = 1,
		 .position = {6, 6},
		 .components = [](ComponentStore& components)
		 {
			 components.add<FlyingRule>();
		 }});
	arena.spawn({.id = 3, .position = {9, 9}});

	CHECK_THROWS(arena.interaction.apply(damage(2, 1, 1)), std::logic_error);
	CHECK_THROWS(arena.interaction.apply(damage(2, 3, 1)), std::logic_error);
	CHECK_THROWS(arena.interaction.apply(damage(2, 2, 1)), std::logic_error);
	CHECK_THROWS(arena.interaction.apply(damage(99, 1, 1)), std::logic_error);
	CHECK(arena.log.empty());
}

TEST(Interaction_every_application_is_a_change_of_the_world)
{
	Arena arena;
	const Unit& source = arena.spawn({.id = 2, .position = {5, 5}});
	const Unit& target = arena.spawn({.id = 1, .position = {6, 5}});
	const uint64_t before = arena.world.activity().changes();

	arena.interaction.apply(damage(2, 1, 1));
	arena.interaction.apply(healing(2, 1, 0));
	CHECK(arena.interaction.applyStatus(source, target, debuff({1, 1}), std::make_unique<Burning>(5)));

	CHECK_EQ(arena.world.activity().changes(), before + 3);
}

TEST(Interaction_status_is_added_and_takes_part_in_later_hits)
{
	Arena arena;
	const Unit& source = arena.spawn({.id = 2, .position = {5, 5}});
	const Unit& target = arena.spawn({.id = 1, .position = {6, 5}});

	CHECK(arena.interaction.applyStatus(source, target, debuff({1, 1}), std::make_unique<ShieldStatus>()));
	CHECK(target.has<ShieldStatus>());

	arena.interaction.apply(damage(2, 1, 5));
	CHECK_EQ(target.get<Health>().current, int64_t{10});
}

TEST(Interaction_status_respects_target_rules)
{
	Arena arena;
	const Unit& source = arena.spawn({.id = 2, .position = {5, 5}});
	const Unit& immune = arena.spawn(
		{.id = 1,
		 .position = {6, 5},
		 .components = [](ComponentStore& components)
		 {
			 components.add<UntargetableRule>();
		 }});
	const Unit& farAway = arena.spawn({.id = 3, .position = {9, 9}});

	CHECK(!arena.interaction.applyStatus(source, immune, debuff({1, 1}), std::make_unique<Burning>(5)));
	CHECK(!immune.has<Burning>());
	CHECK(!arena.interaction.applyStatus(source, farAway, debuff({1, 1}), std::make_unique<Burning>(5)));

	const Unit ghost = makeUnit({.id = 5, .position = {6, 6}});
	CHECK_THROWS(
		static_cast<void>(arena.interaction.applyStatus(source, ghost, debuff({1, 1}), std::make_unique<Burning>(5))),
		std::logic_error);

	const sw::core::TargetSpec notAStatus = meleeAttack();
	CHECK_THROWS(
		static_cast<void>(arena.interaction.applyStatus(source, farAway, notAStatus, std::make_unique<Burning>(5))),
		std::logic_error);
}

TEST(Interaction_reapplied_status_replaces_the_old_one_by_default)
{
	Arena arena;
	const Unit& source = arena.spawn({.id = 2, .position = {5, 5}});
	const Unit& target = arena.spawn({.id = 1, .position = {6, 5}});

	CHECK(arena.interaction.applyStatus(source, target, debuff({1, 1}), std::make_unique<Burning>(5)));
	const Burning* first = target.tryGet<Burning>();
	CHECK(arena.interaction.applyStatus(source, target, debuff({1, 1}), std::make_unique<Burning>(9)));

	CHECK(target.tryGet<Burning>() != first);
	CHECK(target.get<Burning>().expiresAt() == sw::core::Tick{9});
}

TEST(Interaction_reapplied_status_may_keep_itself_and_stack)
{
	Arena arena;
	const Unit& source = arena.spawn({.id = 2, .position = {5, 5}});
	const Unit& target = arena.spawn({.id = 1, .position = {6, 5}});

	CHECK(arena.interaction.applyStatus(source, target, debuff({1, 1}), std::make_unique<StackingPoison>(5)));
	const StackingPoison* first = target.tryGet<StackingPoison>();
	CHECK(arena.interaction.applyStatus(source, target, debuff({1, 1}), std::make_unique<StackingPoison>(8)));

	CHECK(target.tryGet<StackingPoison>() == first);
	CHECK_EQ(target.get<StackingPoison>().stacks, 2);
	CHECK(target.get<StackingPoison>().expiresAt() == sw::core::Tick{8});
}
