#pragma once

#include <Core/Common/Types.hpp>
#include <cstdint>
#include <string>

// События механик Core (SPECIFICATION.md, п. 2). Это доменные события: формат вывода
// они не знают, в io::Events их переводят адаптеры App/Output. Числа — int64_t (п. 1.4).
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
		std::string unitType;  // "Swordsman"
		Position position;

		friend bool operator==(const UnitSpawned&, const UnitSpawned&) = default;
	};

	struct UnitMoved
	{
		UnitId unitId;
		Position position;	// клетка после шага

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
		int64_t damage{};	 // итоговый урон после модификаторов цели
		int64_t targetHp{};	 // HP цели после удара, не меньше 0

		friend bool operator==(const UnitAttacked&, const UnitAttacked&) = default;
	};

	struct UnitHealed
	{
		UnitId healerId;
		UnitId targetId;
		int64_t amount{};  // итоговое лечение после модификаторов цели, может быть 0
		int64_t targetHp{};	 // HP цели после лечения

		friend bool operator==(const UnitHealed&, const UnitHealed&) = default;
	};

	struct UnitDied
	{
		UnitId unitId;

		friend bool operator==(const UnitDied&, const UnitDied&) = default;
	};
}
