#include "Core/Units/TestComponents.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

using sw::core::Health;
using sw::core::Movement;
using sw::core::Position;
using sw::core::Status;
using sw::core::Tag;
using sw::core::Unit;
using sw::core::UnitBlueprint;
using sw::core::UnitId;
using namespace sw::test;

namespace
{
	UnitBlueprint blueprint(std::string type = "Swordsman")
	{
		UnitBlueprint result;
		result.id = UnitId{1};
		result.type = std::move(type);
		result.position = {2, 3};
		return result;
	}
}

TEST(Unit_is_built_from_blueprint)
{
	UnitBlueprint parts = blueprint();
	parts.occupiesCell = false;
	parts.tags.add(Tag("flying"));
	parts.components.add<Health>(10);
	parts.behaviour.push_back(std::make_unique<RequiringAction<>>());

	Unit unit(std::move(parts));

	CHECK_EQ(unit.id(), UnitId{1});
	CHECK_EQ(unit.type(), std::string_view("Swordsman"));
	CHECK_EQ(unit.position(), (Position{2, 3}));
	CHECK(!unit.occupiesCell());
	CHECK(unit.tags().has(Tag("flying")));
	CHECK_EQ(unit.get<Health>().current, int64_t{10});
	CHECK_EQ(unit.behaviour().size(), size_t{1});
	CHECK(!unit.isRemovalRequested());
}

TEST(Unit_dies_when_hp_drops_to_zero_or_below)
{
	UnitBlueprint parts = blueprint();
	parts.components.add<Health>(5);
	Unit unit(std::move(parts));

	CHECK(!unit.isDead());
	unit.get<Health>().current = 0;
	CHECK(unit.isDead());
	unit.get<Health>().current = -3;
	CHECK(unit.isDead());
}

TEST(Unit_without_health_never_dies)
{
	Unit mine(blueprint("Mine"));
	CHECK(!mine.isDead());
	mine.requestRemoval();
	CHECK(mine.isRemovalRequested());
}

TEST(Unit_with_action_missing_a_component_is_not_built)
{
	UnitBlueprint parts = blueprint("Tower");
	parts.behaviour.push_back(std::make_unique<RequiringAction<Movement>>());

	const std::string message = logicErrorMessage([&] { Unit tower(std::move(parts)); });

	CHECK(contains(message, "unit 'Tower' (id 1)"));
	CHECK(contains(message, "RequiringAction<sw::core::Movement>"));
	CHECK(contains(message, "requires component sw::core::Movement"));
}

TEST(Unit_reports_all_missing_components_at_once)
{
	UnitBlueprint parts = blueprint("Hunter");
	parts.components.add<Health>(10);
	parts.behaviour.push_back(std::make_unique<RequiringAction<Strength, Health>>());
	parts.behaviour.push_back(std::make_unique<RequiringAction<Movement>>());

	const std::string message = logicErrorMessage([&] { Unit hunter(std::move(parts)); });

	CHECK(contains(message, "requires component sw::test::Strength"));
	CHECK(contains(message, "requires component sw::core::Movement"));
	CHECK(!contains(message, "requires component sw::core::Health"));
}

TEST(Unit_with_satisfied_requirements_is_built)
{
	UnitBlueprint parts = blueprint();
	parts.components.add<Strength>(2);
	parts.components.add<Movement>(1);
	parts.behaviour.push_back(std::make_unique<RequiringAction<Strength>>());
	parts.behaviour.push_back(std::make_unique<RequiringAction<Movement>>());

	CHECK(logicErrorMessage([&] { Unit swordsman(std::move(parts)); }).empty());
}

TEST(Unit_hook_index_survives_moving_the_unit)
{
	UnitBlueprint parts = blueprint();
	const auto& rule = parts.components.add<RuleComponent>();
	std::vector<Unit> units;
	units.emplace_back(std::move(parts));
	units.emplace_back(blueprint("Other"));

	CHECK_EQ(units[0].components().targetRules().size(), size_t{1});
	CHECK(units[0].components().targetRules()[0] == &rule);
	CHECK(&units[0].get<RuleComponent>() == &rule);
}

TEST(Core_components_reject_invalid_values)
{
	CHECK_THROWS(Health(0), std::logic_error);
	CHECK_THROWS(Health(-1), std::logic_error);
	CHECK_THROWS(Movement(0), std::logic_error);

	const Health health(7);
	CHECK_EQ(health.max, int64_t{7});
	CHECK_EQ(Movement(2).speed, int64_t{2});
}

TEST(Status_expires_at_its_tick)
{
	const Status timed(sw::core::Tick{5});
	CHECK(!timed.isExpired(4));
	CHECK(timed.isExpired(5));
	CHECK(timed.isExpired(6));

	const Status permanent(std::nullopt);
	CHECK(!permanent.isExpired(1'000'000));
}
