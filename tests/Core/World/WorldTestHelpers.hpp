#pragma once

#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace sw::test
{
	struct TestUnit
	{
		uint32_t id;
		core::Position position;
		bool occupiesCell = true;
		std::optional<int64_t> hp = 10;	 // nullopt — юнит без Health, как мина
		std::function<void(core::ComponentStore&)> components = {};	 // дополнительные компоненты
	};

	inline core::Unit makeUnit(const TestUnit& description)
	{
		core::UnitBlueprint blueprint;
		blueprint.id = core::UnitId{description.id};
		blueprint.type = "TestUnit";
		blueprint.position = description.position;
		blueprint.occupiesCell = description.occupiesCell;
		if (description.hp)
		{
			blueprint.components.add<core::Health>(*description.hp);
		}
		if (description.components)
		{
			description.components(blueprint.components);
		}
		return core::Unit(std::move(blueprint));
	}

	inline void kill(core::Unit& unit)
	{
		unit.get<core::Health>().current = 0;
	}
}
