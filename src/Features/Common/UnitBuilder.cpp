#include "UnitBuilder.hpp"

#include <string>
#include <utility>

namespace sw::features
{
	UnitBuilder::UnitBuilder(core::UnitId id, std::string type, core::Position position)
	{
		_blueprint.id = id;
		_blueprint.type = std::move(type);
		_blueprint.position = position;
	}

	core::Unit UnitBuilder::build()
	{
		return core::Unit(std::move(_blueprint));
	}

	core::SpawnRequest UnitBuilder::spawnRequest()
	{
		const core::UnitId id = _blueprint.id;
		const core::Position position = _blueprint.position;
		const bool occupies = _blueprint.occupiesCell;
		return {id, position, occupies, build()};
	}

	core::SpawnRequest UnitBuilder::reject(core::Diagnostics errors) const
	{
		return {_blueprint.id, _blueprint.position, _blueprint.occupiesCell, std::move(errors)};
	}

	void requirePositive(core::Diagnostics& errors, std::string_view name, int64_t value)
	{
		if (value <= 0)
		{
			errors.add("unit.invalid_stat", std::string(name) + " must be positive, got " + std::to_string(value));
		}
	}
}
