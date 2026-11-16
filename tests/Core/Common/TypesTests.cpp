#include "TestFramework/TestFramework.hpp"

#include <Core/Common/Types.hpp>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using sw::core::chebyshev;
using sw::core::Position;
using sw::core::UnitId;

TEST(UnitId_compares_and_hashes_by_value)
{
	CHECK(UnitId{1} == UnitId{1});
	CHECK(UnitId{1} != UnitId{2});
	CHECK(UnitId{1} < UnitId{2});

	std::unordered_set<UnitId> ids{UnitId{1}, UnitId{2}, UnitId{1}};
	CHECK_EQ(ids.size(), size_t{2});
}

TEST(Position_compares_and_hashes_by_coordinates)
{
	const Position a{1, 2};
	const Position sameAsA{1, 2};
	const Position mirrored{2, 1};
	CHECK(a == sameAsA);
	CHECK(a != mirrored);

	std::unordered_map<Position, int> cells;
	cells[{1, 2}] = 1;
	cells[{2, 1}] = 2;
	cells[{1, 2}] = 3;
	CHECK_EQ(cells.size(), size_t{2});
	CHECK_EQ(cells.at({1, 2}), 3);
}

TEST(Position_and_UnitId_print_for_messages)
{
	std::ostringstream stream;
	stream << Position{3, -1} << ' ' << UnitId{7};
	CHECK_EQ(stream.str(), std::string("(3,-1) 7"));
}

TEST(Chebyshev_is_max_of_axis_distances)
{
	CHECK_EQ(chebyshev({4, 4}, {4, 4}), int64_t{0});
	CHECK_EQ(chebyshev({0, 0}, {1, 1}), int64_t{1});
	CHECK_EQ(chebyshev({7, 0}, {3, 0}), int64_t{4});
	CHECK_EQ(chebyshev({5, 0}, {0, 4}), int64_t{5});
	CHECK_EQ(chebyshev({0, 4}, {5, 0}), int64_t{5});
}

TEST(Chebyshev_handles_far_corners_of_the_largest_map)
{
	const int64_t farthest = 4294967294;
	CHECK_EQ(chebyshev({0, 0}, {farthest, farthest}), farthest);
}
