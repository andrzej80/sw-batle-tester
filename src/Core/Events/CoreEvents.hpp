#pragma once

#include <Core/Common/Types.hpp>
#include <cstdint>
#include <string>

namespace sw::core::events
{
	struct MapCreated
	{
		int64_t width{};
		int64_t height{};

		friend bool operator==(const MapCreated&, const MapCreated&) = default;
	};

	struct UnitSpawned
	{
		UnitId unitId;
		std::string unitType;
		Position position;

		friend bool operator==(const UnitSpawned&, const UnitSpawned&) = default;
	};

	struct UnitMoved
	{
		UnitId unitId;
		Position position;

		friend bool operator==(const UnitMoved&, const UnitMoved&) = default;
	};

	struct MarchStarted
	{
		UnitId unitId;
		Position from;
		Position target;

		friend bool operator==(const MarchStarted&, const MarchStarted&) = default;
	};

	struct MarchEnded
	{
		UnitId unitId;
		Position position;

		friend bool operator==(const MarchEnded&, const MarchEnded&) = default;
	};

	struct UnitAttacked
	{
		UnitId attackerId;
		UnitId targetId;
		int64_t damage{};
		int64_t targetHp{};

		friend bool operator==(const UnitAttacked&, const UnitAttacked&) = default;
	};

	struct UnitHealed
	{
		UnitId healerId;
		UnitId targetId;
		int64_t amount{};
		int64_t targetHp{};

		friend bool operator==(const UnitHealed&, const UnitHealed&) = default;
	};

	struct UnitDied
	{
		UnitId unitId;

		friend bool operator==(const UnitDied&, const UnitDied&) = default;
	};
}
