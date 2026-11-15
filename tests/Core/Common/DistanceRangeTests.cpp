#include "TestFramework/TestFramework.hpp"

#include <Core/Common/DistanceRange.hpp>
#include <optional>

using sw::core::DistanceRange;

TEST(DistanceRange_contains_both_bounds)
{
	const DistanceRange range{2, 4};
	CHECK(!range.contains(1));
	CHECK(range.contains(2));
	CHECK(range.contains(3));
	CHECK(range.contains(4));
	CHECK(!range.contains(5));
}

TEST(DistanceRange_with_min_above_max_is_empty)
{
	const DistanceRange single{0, 0};
	const DistanceRange inverted{2, 1};
	CHECK(!single.empty());
	CHECK(inverted.empty());
	CHECK(!inverted.contains(1));
	CHECK(!inverted.contains(2));
}

// Таблица приёмки SPECIFICATION.md, п. 1.3
TEST(DistanceRange_shifted_moves_bounds_independently)
{
	const DistanceRange shooter{2, 4};
	CHECK_EQ(shooter.shifted(-1, -1), (DistanceRange{1, 3}));  // Ворон
	CHECK_EQ(shooter.shifted(0, +2), (DistanceRange{2, 6}));   // бафф максимальной дальности
	CHECK_EQ(shooter.shifted(+1, 0), (DistanceRange{3, 4}));   // сдвиг минимальной границы
}

TEST(DistanceRange_shifted_intersects_with_non_negative_distances)
{
	// [-1, 0] ∩ [0, ∞) = {0, 0}: дистанция 0 в отрезке осталась
	CHECK_EQ((DistanceRange{0, 1}.shifted(-1, -1)), (DistanceRange{0, 0}));
	// [-1, -1] ∩ [0, ∞) пусто: досягаемости не осталось, а не удар по своей клетке
	CHECK((DistanceRange{1, 1}.shifted(-2, -2) == std::nullopt));
	// [5, 4] — пустой отрезок
	CHECK((DistanceRange{2, 4}.shifted(+3, 0) == std::nullopt));
}
