#include "Core/Units/TestComponents.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Units/ComponentStore.hpp>
#include <memory>
#include <stdexcept>
#include <vector>

using sw::core::ComponentStore;
using sw::core::IHitModifier;
using sw::core::ITargetRule;
using namespace sw::test;

TEST(ComponentStore_adds_and_finds_components_by_type)
{
	ComponentStore store;
	store.add<Strength>(5);

	CHECK(store.has<Strength>());
	CHECK(!store.has<RuleComponent>());
	CHECK_EQ(store.get<Strength>().value, int64_t{5});
	CHECK(store.tryGet<RuleComponent>() == nullptr);

	store.get<Strength>().value = 7;
	CHECK_EQ(store.get<Strength>().value, int64_t{7});
}

TEST(ComponentStore_missing_component_names_its_type)
{
	const ComponentStore store;
	const std::string message = logicErrorMessage([&] { static_cast<void>(store.get<Strength>()); });
	CHECK(contains(message, "sw::test::Strength"));
	CHECK(contains(message, "missing"));
}

TEST(ComponentStore_rejects_a_second_component_of_the_same_type)
{
	ComponentStore store;
	store.add<Strength>(1);
	CHECK_THROWS(store.add<Strength>(2), std::logic_error);
	CHECK_EQ(store.get<Strength>().value, int64_t{1});
}

TEST(ComponentStore_indexes_hooks_in_order_of_addition)
{
	ComponentStore store;
	store.add<Strength>(1);
	const auto& rule = store.add<RuleComponent>();
	const auto& modifier = store.add<ModifierComponent>();
	auto& both = store.add<RuleAndModifierComponent>();

	const std::vector<const ITargetRule*> expectedRules{&rule, &both};
	const std::vector<IHitModifier*> expectedModifiers{const_cast<ModifierComponent*>(&modifier), &both};
	CHECK(store.targetRules() == expectedRules);
	CHECK(store.hitModifiers() == expectedModifiers);
}

TEST(ComponentStore_remove_drops_component_from_hook_index)
{
	ComponentStore store;
	store.add<RuleComponent>();
	auto& both = store.add<RuleAndModifierComponent>();

	CHECK(store.remove<RuleComponent>());
	CHECK(!store.has<RuleComponent>());
	CHECK_EQ(store.targetRules().size(), size_t{1});
	CHECK(store.targetRules()[0] == &both);

	CHECK(store.remove<RuleAndModifierComponent>());
	CHECK(store.targetRules().empty());
	CHECK(store.hitModifiers().empty());

	CHECK(!store.remove<RuleComponent>());
}

TEST(ComponentStore_accepts_components_known_only_at_runtime)
{
	// Так добавляются статусы: Interaction получает std::unique_ptr<Status>
	ComponentStore store;
	std::unique_ptr<sw::core::IComponent> status = std::make_unique<ModifierComponent>();
	store.add(typeid(ModifierComponent), std::move(status));

	CHECK(store.has<ModifierComponent>());
	CHECK_EQ(store.hitModifiers().size(), size_t{1});
	CHECK_THROWS(store.add(typeid(Strength), nullptr), std::logic_error);
}
