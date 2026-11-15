#include "Core/Simulation/SimulationTestHelpers.hpp"
#include "Core/World/WorldTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Map/CellFeatures.hpp>
#include <memory>
#include <stdexcept>

using sw::core::IEnterTrigger;
using sw::core::IMoveCostModifier;
using sw::core::Position;
using sw::core::TurnContext;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::test;

namespace
{
	struct Swamp final : IMoveCostModifier
	{
		int64_t moveCost(const Unit&, int64_t cost) const override
		{
			return cost + 2;  // вход стоит 3
		}
	};

	struct Trap final : IEnterTrigger
	{
		explicit Trap(std::vector<std::string>& log) :
				log(log)
		{}

		void onEnter(const Unit& unit, TurnContext& context) override
		{
			log.push_back("trap " + std::to_string(unit.id().value) + " at tick " + std::to_string(context.tick()));
		}

		std::vector<std::string>& log;
	};

	using Log = std::vector<std::string>;
}

TEST(MovementService_steps_into_a_free_neighbour_cell)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});

	CHECK(arena.context().movement().stepTo(unit, {1, 1}, arena.context()));

	CHECK_EQ(unit.position(), (Position{1, 1}));
	CHECK(!arena.world.map().isOccupied({0, 0}));
	CHECK(arena.world.map().occupant({1, 1}) == UnitId{1});
	CHECK(arena.log == (Log{"[1] moved 1 1,1"}));
}

TEST(MovementService_refuses_illegal_steps_without_changes)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	const Unit& tower = arena.spawn({.id = 2, .position = {5, 5}, .speed = std::nullopt});
	Unit& corpse = arena.spawn({.id = 3, .position = {1, 0}});
	kill(corpse);
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!arena.context().movement().stepTo(unit, {2, 2}, arena.context()));	// не соседняя
	CHECK(!arena.context().movement().stepTo(unit, {-1, 0}, arena.context()));	// вне карты
	CHECK(!arena.context().movement().stepTo(unit, {1, 0}, arena.context()));	// занята мёртвым
	CHECK(!arena.context().movement().stepTo(tower, {5, 6}, arena.context()));	// статичен всегда

	CHECK_EQ(unit.position(), (Position{0, 0}));
	CHECK_EQ(tower.position(), (Position{5, 5}));
	CHECK_EQ(arena.world.activity().changes(), changes);
	CHECK(arena.log.empty());
}

TEST(MovementService_unit_without_cell_occupancy_passes_over_others)
{
	SimArena arena(10, 10);
	arena.spawn({.id = 1, .position = {1, 0}});
	const Unit& raven = arena.spawn({.id = 2, .position = {0, 0}, .occupiesCell = false});

	CHECK(arena.context().movement().stepTo(raven, {1, 0}, arena.context()));
	CHECK(arena.world.map().occupant({1, 0}) == UnitId{1});
}

TEST(MovementService_fires_enter_triggers_of_the_cell)
{
	SimArena arena(10, 10);
	Log traps;
	arena.world.map().addCellFeature({1, 0}, std::make_shared<Trap>(traps));
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});

	CHECK(arena.context().movement().stepTo(unit, {1, 0}, arena.context()));
	CHECK(traps == (Log{"trap 1 at tick 1"}));
}

TEST(MovementService_first_march_starts_at_once_and_next_ones_wait_in_queue)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	arena.march(unit, {3, 0});
	arena.march(unit, {3, 3});

	CHECK(arena.log == (Log{"[1] march started 1 0,0->3,0"}));
	CHECK_EQ(unit.get<sw::core::Movement>().marches.size(), size_t{2});
}

TEST(MarchStep_walks_straight_and_ends_march_on_arrival)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn({.id = 2, .position = {9, 9}, .speed = std::nullopt});	// второй юнит: иначе стоп сразу
	arena.march(unit, {3, 0});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(arena.simulation.runTick() == std::nullopt);

	CHECK(
		arena.log
		== (Log{
			"[1] march started 1 0,0->3,0",
			"[2] moved 1 1,0",
			"[3] moved 1 2,0",
			"[4] moved 1 3,0",
			"[4] march ended 1 3,0"}));
	CHECK(unit.get<sw::core::Movement>().marches.empty());
}

TEST(MarchStep_goes_around_an_occupied_cell)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn({.id = 2, .position = {1, 0}, .speed = std::nullopt});	// стоит на прямом пути
	arena.march(unit, {2, 0});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK_EQ(unit.position(), (Position{1, 1}));  // единственная клетка, приближающая к цели
}

TEST(MarchStep_waits_when_no_cell_gets_closer)
{
	SimArena arena(3, 1);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}});
	arena.spawn({.id = 2, .position = {1, 0}, .speed = std::nullopt});
	arena.march(unit, {2, 0});
	const uint64_t changes = arena.world.activity().changes();

	CHECK(!arena.context().movement().stepTo(unit, {1, 0}, arena.context()));
	sw::core::MarchStep step;
	CHECK(!step.tryExecute(unit, arena.context()));

	CHECK_EQ(unit.position(), (Position{0, 0}));
	CHECK_EQ(arena.world.activity().changes(), changes);
	CHECK_EQ(unit.get<sw::core::Movement>().marches.size(), size_t{1});	 // марш не отменён
}

TEST(MarchStep_speed_two_moves_two_cells_per_tick)
{
	SimArena arena(10, 10);
	const Unit& raven = arena.spawn({.id = 1, .position = {0, 0}, .speed = 2, .occupiesCell = false});
	arena.spawn({.id = 2, .position = {9, 9}, .speed = std::nullopt});
	arena.march(raven, {5, 0});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(arena.log == (Log{"[1] march started 1 0,0->5,0", "[2] moved 1 1,0", "[2] moved 1 2,0"}));
}

TEST(MarchStep_first_step_is_always_allowed_but_expensive_cell_stops_the_rest)
{
	SimArena arena(10, 10);
	arena.world.map().addCellFeature({1, 0}, std::make_shared<Swamp>());
	arena.world.map().addCellFeature({4, 0}, std::make_shared<Swamp>());
	const Unit& slow = arena.spawn({.id = 1, .position = {0, 0}, .speed = 1});
	const Unit& fast = arena.spawn({.id = 2, .position = {3, 1}, .speed = 2});
	arena.march(slow, {2, 0});
	arena.march(fast, {6, 1});

	CHECK(arena.simulation.runTick() == std::nullopt);
	// slow: болото стоит 3 при скорости 1, но первый шаг разрешён
	CHECK_EQ(slow.position(), (Position{1, 0}));
	// fast: прямой шаг (4,1) стоит 1, следующий прямой (5,1) — обычный: оба шага
	CHECK_EQ(fast.position(), (Position{5, 1}));
}

TEST(MarchStep_second_step_into_expensive_cell_waits_for_next_tick)
{
	SimArena arena(10, 10);
	arena.world.map().addCellFeature({2, 0}, std::make_shared<Swamp>());
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}, .speed = 2});
	arena.spawn({.id = 2, .position = {9, 9}, .speed = std::nullopt});
	arena.march(unit, {3, 0});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK_EQ(unit.position(), (Position{1, 0}));  // (2,0) стоит 3 > оставшегося бюджета 1
	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK_EQ(unit.position(), (Position{2, 0}));  // первый шаг разрешён, но съел весь бюджет
	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK_EQ(unit.position(), (Position{3, 0}));
}

TEST(MarchStep_next_march_starts_in_the_same_tick_and_moves_from_the_next)
{
	SimArena arena(10, 10);
	const Unit& unit = arena.spawn({.id = 1, .position = {0, 0}, .speed = 2});
	arena.spawn({.id = 2, .position = {9, 9}, .speed = std::nullopt});
	arena.march(unit, {1, 0});
	arena.march(unit, {1, 3});

	CHECK(arena.simulation.runTick() == std::nullopt);
	CHECK(arena.simulation.runTick() == std::nullopt);

	CHECK(
		arena.log
		== (Log{
			"[1] march started 1 0,0->1,0",
			"[2] moved 1 1,0",
			"[2] march ended 1 1,0",
			"[2] march started 1 1,0->1,3",
			"[3] moved 1 1,1",
			"[3] moved 1 1,2"}));
}
