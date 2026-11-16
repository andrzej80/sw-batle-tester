#include "Core/World/WorldTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Map/Map.hpp>
#include <stdexcept>
#include <vector>

using sw::core::Map;
using sw::core::Position;
using sw::core::Unit;
using sw::core::UnitId;
using namespace sw::test;

TEST(Map_requires_positive_size)
{
	CHECK_THROWS(Map(0, 5), std::logic_error);
	CHECK_THROWS(Map(5, 0), std::logic_error);
	CHECK_THROWS(Map(-1, 5), std::logic_error);
}

TEST(Map_contains_cells_from_zero_to_size_minus_one)
{
	const Map map(10, 5);
	CHECK(map.contains({0, 0}));
	CHECK(map.contains({9, 4}));
	CHECK(!map.contains({10, 4}));
	CHECK(!map.contains({9, 5}));
	CHECK(!map.contains({-1, 0}));
}

TEST(Map_occupy_and_release)
{
	Map map(10, 10);
	map.occupy({2, 3}, UnitId{1});
	CHECK(map.isOccupied({2, 3}));
	CHECK(map.occupant({2, 3}) == UnitId{1});
	CHECK(map.occupant({3, 3}) == std::nullopt);

	CHECK_THROWS(map.occupy({2, 3}, UnitId{2}), std::logic_error);
	CHECK_THROWS(map.occupy({10, 0}, UnitId{2}), std::logic_error);
	CHECK_THROWS(map.release({2, 3}, UnitId{2}), std::logic_error);

	map.release({2, 3}, UnitId{1});
	CHECK(!map.isOccupied({2, 3}));
}

TEST(Map_neighbours_of_a_corner_do_not_wrap_around)
{
	const Map map(4294967295, 4294967295);
	const std::vector<Position> expected{{1, 0}, {0, 1}, {1, 1}};
	CHECK(map.neighbourCells({0, 0}) == expected);
}

TEST(Map_neighbours_inside_are_all_eight_in_fixed_order)
{
	const Map map(10, 10);
	const std::vector<Position> expected{{4, 4}, {5, 4}, {6, 4}, {4, 5}, {6, 5}, {4, 6}, {5, 6}, {6, 6}};
	CHECK(map.neighbourCells({5, 5}) == expected);
}

TEST(Map_largest_map_is_stored_sparsely)
{
	Map map(4294967295, 4294967295);
	const Position farCorner{4294967294, 4294967294};
	CHECK(map.contains(farCorner));
	map.occupy(farCorner, UnitId{1});
	CHECK(map.occupant(farCorner) == UnitId{1});
	CHECK(!map.contains({4294967295, 0}));
}
