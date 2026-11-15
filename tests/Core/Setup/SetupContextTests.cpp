#include "Core/World/WorldTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Events/CoreEvents.hpp>
#include <Core/Events/EventBus.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Core/World/World.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using sw::core::Diagnostic;
using sw::core::EventBus;
using sw::core::Position;
using sw::core::Result;
using sw::core::SetupContext;
using sw::core::SpawnRequest;
using sw::core::UnitId;
using namespace sw::test;

namespace
{
	using Codes = std::vector<std::string>;

	Codes codes(const Result<void>& result)
	{
		Codes list;
		for (const Diagnostic& error : result.errors())
		{
			list.push_back(error.code);
		}
		return list;
	}

	// Заявка на спавн собранного юнита
	SpawnRequest valid(const TestUnit& description)
	{
		return {UnitId{description.id}, description.position, description.occupiesCell, makeUnit(description)};
	}

	// Заявка, в которой фабрика нашла ошибку статов
	SpawnRequest invalidStats(uint32_t id, Position position)
	{
		return {UnitId{id}, position, true, Diagnostic{"unit.invalid_stat", "hp must be positive, got 0"}};
	}

	// Подвижный юнит: Movement нужен для маршей
	TestUnit mover(uint32_t id, Position position)
	{
		return {
			.id = id,
			.position = position,
			.components = [](sw::core::ComponentStore& components)
			{
				components.add<sw::core::Movement>(1);
			}};
	}

	struct Setup
	{
		Setup()
		{
			namespace events = sw::core::events;
			bus.subscribe<events::MapCreated>(
				[this](const events::MapCreated& e)
				{ log.push_back("map " + std::to_string(e.width) + "x" + std::to_string(e.height)); });
			bus.subscribe<events::UnitSpawned>(
				[this](const events::UnitSpawned& e)
				{ log.push_back("spawned " + std::to_string(e.unitId.value) + " " + e.unitType); });
			bus.subscribe<events::MarchStarted>([this](const events::MarchStarted& e)
												{ log.push_back("march " + std::to_string(e.unitId.value)); });
		}

		EventBus bus;
		SetupContext context{bus};
		std::vector<std::string> log;
	};
}

TEST(SetupContext_creates_the_map)
{
	Setup setup;
	CHECK(setup.context.createMap(10, 5).ok());
	CHECK(setup.context.world() != nullptr);
	CHECK_EQ(setup.context.world()->map().width(), int64_t{10});
	CHECK(setup.log == (std::vector<std::string>{"map 10x5"}));
	CHECK(setup.context.finish().ok());
}

TEST(SetupContext_rejects_invalid_and_repeated_map)
{
	Setup setup;
	CHECK(codes(setup.context.createMap(0, 10)) == (Codes{"map.invalid_size"}));
	CHECK(setup.context.world() == nullptr);
	CHECK(codes(setup.context.createMap(10, 10)) == (Codes{"map.already_created"}));
	CHECK(codes(setup.context.createMap(0, 0)) == (Codes{"map.already_created", "map.invalid_size"}));
	CHECK(setup.log.empty());
}

// Согласовано: после CREATE_MAP с ошибкой map.not_created не выводится,
// но ошибки самих спавнов — выводятся
TEST(SetupContext_failed_map_suppresses_missing_map_errors)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(0, 10));

	CHECK(setup.context.spawn(valid({.id = 1, .position = {0, 0}})).ok());
	CHECK(codes(setup.context.spawn(invalidStats(2, {1, 1}))) == (Codes{"unit.invalid_stat"}));
	CHECK(setup.context.orderMarch(UnitId{1}, {5, 5}).ok());
	CHECK(setup.context.finish().ok());
}

TEST(SetupContext_command_before_the_map_reports_missing_map_once)
{
	Setup setup;
	CHECK(codes(setup.context.spawn(valid({.id = 1, .position = {0, 0}}))) == (Codes{"map.not_created"}));
	CHECK(setup.context.spawn(valid({.id = 2, .position = {1, 0}})).ok());
	CHECK(setup.context.orderMarch(UnitId{1}, {5, 5}).ok());  // спавн отклонён — последствие
	CHECK(setup.context.finish().ok());						  // уже сообщено
}

TEST(SetupContext_scenario_without_map_is_an_error)
{
	Setup setup;
	CHECK(codes(setup.context.finish()) == (Codes{"map.not_created"}));
}

TEST(SetupContext_spawns_unit_and_publishes_it)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	CHECK(setup.context.spawn(valid({.id = 3, .position = {2, 2}})).ok());

	CHECK(setup.context.world()->units().find(UnitId{3}) != nullptr);
	CHECK(setup.context.world()->map().occupant({2, 2}) == UnitId{3});
	CHECK(setup.log == (std::vector<std::string>{"map 10x10", "spawned 3 TestUnit"}));
}

TEST(SetupContext_duplicate_id_points_to_the_first_unit)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	CHECK(setup.context.spawn(valid(mover(1, {0, 0}))).ok());

	const Result<void> result = setup.context.spawn(valid({.id = 1, .position = {5, 5}}));
	CHECK(codes(result) == (Codes{"unit.duplicate_id"}));
	CHECK(result.errors()[0].relatedUnit == UnitId{1});
	// Марш на этот id относится к первому юниту и проверяется как обычно
	CHECK(setup.context.orderMarch(UnitId{1}, {3, 0}).ok());
	CHECK(codes(setup.context.orderMarch(UnitId{1}, {3, 0})) == (Codes{"march.same_position"}));
}

TEST(SetupContext_spawn_outside_the_map)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	const Result<void> result = setup.context.spawn(valid({.id = 1, .position = {10, 3}}));
	CHECK(codes(result) == (Codes{"unit.out_of_bounds"}));
	CHECK(result.errors()[0].message.find("(10,3)") != std::string::npos);
	CHECK(result.errors()[0].message.find("10x10") != std::string::npos);
}

TEST(SetupContext_spawn_into_occupied_cell)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	CHECK(setup.context.spawn(valid({.id = 2, .position = {9, 0}})).ok());

	const Result<void> result = setup.context.spawn(valid({.id = 7, .position = {9, 0}}));
	CHECK(codes(result) == (Codes{"unit.cell_occupied"}));
	CHECK(result.errors()[0].relatedUnit == UnitId{2});
	// Юниту, не занимающему клетку, можно
	CHECK(setup.context.spawn(valid({.id = 8, .position = {9, 0}, .occupiesCell = false})).ok());
}

// Приёмка п. 7: у спавна с плохими статами и занятой клеткой — обе ошибки
TEST(SetupContext_reports_all_errors_of_one_spawn)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	CHECK(setup.context.spawn(valid({.id = 1, .position = {4, 4}})).ok());

	CHECK(
		codes(setup.context.spawn(invalidStats(1, {4, 4})))
		== (Codes{"unit.invalid_stat", "unit.duplicate_id", "unit.cell_occupied"}));
	CHECK(codes(setup.context.spawn(invalidStats(2, {4, 4}))) == (Codes{"unit.invalid_stat", "unit.cell_occupied"}));
}

TEST(SetupContext_march_for_missing_or_rejected_unit)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	// MARCH до SPAWN — юнита ещё нет
	CHECK(codes(setup.context.orderMarch(UnitId{5}, {1, 1})) == (Codes{"march.unit_not_found"}));

	static_cast<void>(setup.context.spawn(invalidStats(6, {0, 0})));
	CHECK(setup.context.orderMarch(UnitId{6}, {1, 1}).ok());  // последствие отклонённого спавна
}

TEST(SetupContext_static_unit_cannot_march)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	static_cast<void>(setup.context.spawn(valid({.id = 1, .position = {0, 0}})));  // без Movement

	const Result<void> result = setup.context.orderMarch(UnitId{1}, {1, 1});
	CHECK(codes(result) == (Codes{"march.unit_cannot_move"}));
	CHECK(result.errors()[0].message.find("TestUnit") != std::string::npos);
}

TEST(SetupContext_march_outside_the_map)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	static_cast<void>(setup.context.spawn(valid(mover(1, {0, 0}))));
	CHECK(codes(setup.context.orderMarch(UnitId{1}, {0, 10})) == (Codes{"march.out_of_bounds"}));
}

TEST(SetupContext_march_to_where_the_unit_will_already_be)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	static_cast<void>(setup.context.spawn(valid(mover(1, {2, 2}))));

	CHECK(codes(setup.context.orderMarch(UnitId{1}, {2, 2})) == (Codes{"march.same_position"}));
	CHECK(setup.context.orderMarch(UnitId{1}, {5, 5}).ok());
	// Для очереди «текущая позиция» — конец предыдущего марша
	CHECK(codes(setup.context.orderMarch(UnitId{1}, {5, 5})) == (Codes{"march.same_position"}));
	CHECK(setup.context.orderMarch(UnitId{1}, {2, 2}).ok());  // обратно — уже не то же место
}

TEST(SetupContext_first_march_starts_now_and_next_ones_queue)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	static_cast<void>(setup.context.spawn(valid(mover(1, {0, 0}))));
	CHECK(setup.context.orderMarch(UnitId{1}, {9, 0}).ok());
	CHECK(setup.context.orderMarch(UnitId{1}, {9, 9}).ok());

	CHECK(setup.log == (std::vector<std::string>{"map 10x10", "spawned 1 TestUnit", "march 1"}));
	CHECK_EQ(setup.context.world()->units().find(UnitId{1})->get<sw::core::Movement>().marches.size(), size_t{2});
}

TEST(SetupContext_spawn_request_must_match_the_unit)
{
	Setup setup;
	static_cast<void>(setup.context.createMap(10, 10));
	SpawnRequest mismatched = valid({.id = 1, .position = {0, 0}});
	mismatched.position = {3, 3};
	CHECK_THROWS(static_cast<void>(setup.context.spawn(std::move(mismatched))), std::logic_error);
}
