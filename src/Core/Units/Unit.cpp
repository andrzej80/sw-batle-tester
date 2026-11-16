#include "Unit.hpp"

#include "ComponentRequirements.hpp"
#include "CoreComponents.hpp"

#include <Core/Common/TypeName.hpp>
#include <stdexcept>
#include <utility>

namespace sw::core
{
	namespace
	{
		void checkRequirements(const UnitBlueprint& blueprint)
		{
			std::string problems;
			for (const std::unique_ptr<IAction>& action : blueprint.behaviour)
			{
				if (!action)
				{
					problems += "\n  null action in behaviour";
					continue;
				}
				ComponentRequirements requirements;
				action->requirements(requirements);
				for (const std::string& component : requirements.missingIn(blueprint.components))
				{
					problems += "\n  " + typeName(*action) + " requires component " + component;
				}
			}
			if (!problems.empty())
			{
				throw std::logic_error(
					"unit '" + blueprint.type + "' (id " + std::to_string(blueprint.id.value)
					+ ") is assembled incorrectly:" + problems);
			}
		}
	}

	Unit::Unit(UnitBlueprint blueprint) :
			_id(blueprint.id),
			_position(blueprint.position),
			_occupiesCell(blueprint.occupiesCell)
	{
		checkRequirements(blueprint);
		_type = std::move(blueprint.type);
		_tags = std::move(blueprint.tags);
		_components = std::move(blueprint.components);
		_behaviour = std::move(blueprint.behaviour);
	}

	bool Unit::isDead() const
	{
		const auto* health = tryGet<Health>();
		return health != nullptr && health->current <= 0;
	}
}
