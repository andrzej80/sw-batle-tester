#pragma once

#include <Core/Common/Diagnostic.hpp>
#include <Core/Common/Tag.hpp>
#include <Core/Common/Types.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <Core/Units/Unit.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace sw::features
{
	class UnitBuilder
	{
	public:
		UnitBuilder(core::UnitId id, std::string type, core::Position position);

		UnitBuilder& occupiesCell(bool occupies)
		{
			_blueprint.occupiesCell = occupies;
			return *this;
		}

		UnitBuilder& tag(core::Tag tag)
		{
			_blueprint.tags.add(tag);
			return *this;
		}

		template <class C, class... Args>
		UnitBuilder& with(Args&&... args)
		{
			_blueprint.components.add<C>(std::forward<Args>(args)...);
			return *this;
		}

		template <class Action, class... Args>
		UnitBuilder& action(Args&&... args)
		{
			_blueprint.behaviour.push_back(std::make_unique<Action>(std::forward<Args>(args)...));
			return *this;
		}

		core::Unit build();

		core::SpawnRequest spawnRequest();

		core::SpawnRequest reject(core::Diagnostics errors) const;

	private:
		core::UnitBlueprint _blueprint;
	};

	void requirePositive(core::Diagnostics& errors, std::string_view name, int64_t value);
}
