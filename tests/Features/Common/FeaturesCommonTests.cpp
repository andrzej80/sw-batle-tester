#include "Features/FeatureTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Units/ComponentRequirements.hpp>
#include <Features/Common/Attacks.hpp>
#include <Features/Common/CommonTags.hpp>
#include <Features/Common/TargetSpecs.hpp>
#include <Features/Common/ValueSource.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using sw::core::DistanceRange;
using sw::core::Effect;
using sw::core::Intent;
using sw::core::Position;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::features;
using namespace sw::test;

namespace
{
	using Lines = std::vector<std::string>;

	UnitBuilder shooter(uint32_t id, Position position, int64_t range)
	{
		UnitBuilder builder(UnitId{id}, "Shooter", position);
		builder.with<sw::core::Health>(10).with<Agility>(5).with<Range>(range).action<RangedAttack>(
			stat<Agility>, distance(2, stat<Range>), RequireNoAdjacentOccupying{});
		return builder;
	}

	bool act(Unit& unit, sw::core::TurnContext& context)
	{
		return unit.behaviour().front()->tryExecute(unit, context);
	}
}

TEST(TargetSpecs_name_the_four_kinds_of_effects)
{
	CHECK(attack({tags::Melee}, {1, 1}).intent == Intent::Hostile);
	CHECK(attack({tags::Melee}, {1, 1}).effect == Effect::Damage);
	CHECK(heal({}, {0, 2}).intent == Intent::Beneficial);
	CHECK(heal({}, {0, 2}).effect == Effect::Heal);
	CHECK(buff({}, {0, 1}).effect == Effect::Status);
	CHECK(buff({}, {0, 1}).intent == Intent::Beneficial);
	CHECK(debuff({}, {0, 1}).intent == Intent::Hostile);
	CHECK(tags::Melee == sw::core::Tag("melee"));
}

TEST(Distance_reads_stats_at_the_moment_of_use)
{
	const Distance shot = distance(2, stat<Range>);
	UnitBuilder near(UnitId{1}, "Near", {0, 0});
	near.with<Range>(3);
	UnitBuilder far(UnitId{2}, "Far", {0, 0});
	far.with<Range>(7);
	Unit nearUnit = near.build();
	const Unit farUnit = far.build();

	CHECK_EQ(shot.resolve(nearUnit), (DistanceRange{2, 3}));
	CHECK_EQ(shot.resolve(farUnit), (DistanceRange{2, 7}));

	nearUnit.get<Range>().value = 1;
	CHECK(shot.resolve(nearUnit).empty());
	CHECK_EQ(distance(2, 5).resolve(farUnit), (DistanceRange{2, 5}));
}

TEST(ValueSource_declares_required_stats)
{
	sw::core::ComponentRequirements requirements;
	distance(2, stat<Range>).declare(requirements);
	ValueSource(stat<Agility>).declare(requirements);
	ValueSource(5).declare(requirements);

	UnitBuilder builder(UnitId{1}, "Bare", {0, 0});
	builder.with<Range>(4);
	const Unit unit = builder.build();
	CHECK(requirements.missingIn(unit.components()) == (std::vector<std::string>{"sw::features::Agility"}));
}

TEST(UnitBuilder_assembles_the_unit)
{
	UnitBuilder builder(UnitId{3}, "Raven", {4, 5});
	builder.occupiesCell(false).tag(tags::Flying).with<sw::core::Health>(10).with<Agility>(2);
	const Unit raven = builder.build();

	CHECK_EQ(raven.id(), UnitId{3});
	CHECK_EQ(raven.type(), std::string_view("Raven"));
	CHECK_EQ(raven.position(), (Position{4, 5}));
	CHECK(!raven.occupiesCell());
	CHECK(raven.tags().has(tags::Flying));
	CHECK_EQ(raven.get<Agility>().value, int64_t{2});
}

TEST(UnitBuilder_rejects_action_without_its_stat)
{
	UnitBuilder builder(UnitId{1}, "Swordsman", {0, 0});
	builder.with<sw::core::Health>(10).action<MeleeAttack>(stat<Strength>);

	std::string message;
	try
	{
		static_cast<void>(builder.build());
	}
	catch (const std::logic_error& error)
	{
		message = error.what();
	}
	CHECK(message.find("unit 'Swordsman' (id 1)") != std::string::npos);
	CHECK(message.find("sw::features::MeleeAttack requires component sw::features::Strength") != std::string::npos);
}

TEST(UnitBuilder_spawn_request_carries_identity_with_or_without_the_unit)
{
	UnitBuilder valid(UnitId{1}, "Dummy", {2, 2});
	valid.with<sw::core::Health>(1);
	const sw::core::SpawnRequest built = valid.spawnRequest();
	CHECK(built.unit.ok());
	CHECK_EQ(built.id, UnitId{1});

	UnitBuilder broken(UnitId{2}, "Dummy", {3, 3});
	broken.occupiesCell(false);
	sw::core::Diagnostics errors;
	requirePositive(errors, "hp", 0);
	requirePositive(errors, "strength", 5);
	const sw::core::SpawnRequest rejected = broken.reject(errors);

	CHECK(!rejected.unit.ok());
	CHECK_EQ(rejected.position, (Position{3, 3}));
	CHECK(!rejected.occupiesCell);
	CHECK_EQ(rejected.unit.errors().size(), size_t{1});
	CHECK_EQ(rejected.unit.errors()[0].code, std::string("unit.invalid_stat"));
	CHECK_EQ(rejected.unit.errors()[0].message, std::string("hp must be positive, got 0"));
}

TEST(MeleeAttack_hits_an_adjacent_unit_with_the_stat_value)
{
	FeatureArena arena(10, 10);
	UnitBuilder builder(UnitId{1}, "Swordsman", {5, 5});
	builder.with<sw::core::Health>(10).with<Strength>(3).action<MeleeAttack>(stat<Strength>);
	Unit& swordsman = arena.place(builder);
	arena.dummy(2, {7, 5});
	arena.dummy(3, {6, 6});

	CHECK(act(swordsman, arena.context()));
	CHECK(arena.attacks == (Lines{"1->3 damage=3 hp=7"}));
}

TEST(MeleeAttack_without_neighbours_does_nothing)
{
	FeatureArena arena(10, 10);
	UnitBuilder builder(UnitId{1}, "Swordsman", {5, 5});
	builder.with<sw::core::Health>(10).with<Strength>(3).action<MeleeAttack>(stat<Strength>);
	Unit& swordsman = arena.place(builder);
	arena.dummy(2, {9, 9});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(swordsman, arena.context()));
	CHECK(arena.attacks.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(MeleeAttack_negative_stat_hits_for_zero)
{
	FeatureArena arena(10, 10);
	UnitBuilder builder(UnitId{1}, "Cursed", {5, 5});
	builder.with<sw::core::Health>(10).with<Strength>(3).action<MeleeAttack>(stat<Strength>);
	Unit& cursed = arena.place(builder);
	arena.dummy(2, {5, 6});
	cursed.get<Strength>().value = -4;

	CHECK(act(cursed, arena.context()));
	CHECK(arena.attacks == (Lines{"1->2 damage=0 hp=10"}));
}

TEST(MeleeAttack_by_default_does_not_reach_its_own_cell)
{
	FeatureArena arena(10, 10);
	UnitBuilder builder(UnitId{1}, "Swordsman", {5, 5});
	builder.with<sw::core::Health>(10).with<Strength>(3).action<MeleeAttack>(stat<Strength>);
	Unit& swordsman = arena.place(builder);
	arena.dummy(2, {5, 5}, /*occupiesCell*/ false);
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!act(swordsman, arena.context()));
	CHECK(arena.attacks.empty());
	CHECK_EQ(arena.world.activity().changes(), changes);
}

TEST(MeleeAttack_with_reach_from_zero_hits_on_its_own_cell_and_adjacent)
{
	FeatureArena arena(10, 10);
	UnitBuilder builder(UnitId{1}, "Raven", {5, 5});
	builder.occupiesCell(false).with<sw::core::Health>(10).with<Agility>(4).action<MeleeAttack>(
		stat<Agility>, distance(0, 1));
	Unit& raven = arena.place(builder);
	Unit& onCell = arena.dummy(2, {5, 5});

	CHECK(act(raven, arena.context()));
	CHECK(arena.attacks == (Lines{"1->2 damage=4 hp=6"}));

	kill(onCell);
	arena.dummy(3, {6, 4});
	arena.dummy(4, {7, 5});
	CHECK(act(raven, arena.context()));
	CHECK(arena.attacks == (Lines{"1->2 damage=4 hp=6", "1->3 damage=4 hp=6"}));
}

TEST(MeleeAttack_declares_stats_of_its_reach)
{
	const MeleeAttack claw(stat<Agility>, distance(0, stat<Range>));
	sw::core::ComponentRequirements requirements;
	claw.requirements(requirements);

	UnitBuilder builder(UnitId{1}, "Bare", {0, 0});
	builder.with<Agility>(4);
	const Unit unit = builder.build();
	CHECK(requirements.missingIn(unit.components()) == (std::vector<std::string>{"sw::features::Range"}));
}

TEST(RangedAttack_shoots_within_the_stat_range)
{
	FeatureArena arena(10, 10);
	Unit& hunter = arena.place(shooter(1, {0, 0}, 4).with<Strength>(1));
	arena.dummy(2, {5, 0});
	arena.dummy(3, {4, 4});

	CHECK(act(hunter, arena.context()));
	CHECK(arena.attacks == (Lines{"1->3 damage=5 hp=5"}));
}

TEST(RangedAttack_does_not_shoot_with_an_occupying_neighbour)
{
	FeatureArena arena(10, 10);
	Unit& hunter = arena.place(shooter(1, {0, 0}, 4));
	arena.dummy(2, {3, 0});
	arena.dummy(3, {1, 1});

	CHECK(!act(hunter, arena.context()));
	CHECK(arena.attacks.empty());
}

TEST(RangedAttack_is_not_blocked_by_flying_or_dead_neighbours)
{
	FeatureArena arena(10, 10);
	Unit& hunter = arena.place(shooter(1, {0, 0}, 4));
	arena.dummy(2, {1, 0}, /*occupiesCell*/ false);
	Unit& corpse = arena.dummy(3, {0, 1});
	kill(corpse);
	arena.dummy(4, {3, 3});

	CHECK(act(hunter, arena.context()));
	CHECK(arena.attacks == (Lines{"1->4 damage=5 hp=5"}));
}

TEST(RangedAttack_ignores_targets_closer_than_the_minimum)
{
	FeatureArena arena(10, 10);
	UnitBuilder builder(UnitId{1}, "Tower", {5, 5});
	builder.with<sw::core::Health>(10).with<Power>(7).action<RangedAttack>(stat<Power>, distance(2, 5));
	Unit& tower = arena.place(builder);
	arena.dummy(2, {5, 6}, /*occupiesCell*/ false);

	CHECK(!act(tower, arena.context()));
	arena.dummy(3, {5, 8});
	CHECK(act(tower, arena.context()));
	CHECK(arena.attacks == (Lines{"1->3 damage=7 hp=3"}));
}
