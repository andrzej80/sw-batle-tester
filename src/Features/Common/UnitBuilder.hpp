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
	// Сборка юнита в фабрике фичи (SPECIFICATION.md, п. 9.1):
	//
	//   UnitBuilder builder(spec.id, "Hunter", spec.position);
	//   if (!errors.empty())
	//       return builder.reject(errors);
	//   return builder.with<core::Health>(spec.hp)
	//       .action<MeleeAttack>(stat<Strength>)
	//       .spawnRequest();
	//
	// Требования действий к компонентам проверяет конструктор core::Unit: недостающий
	// компонент — std::logic_error при сборке, а не на 500-м тике.
	class UnitBuilder
	{
	public:
		UnitBuilder(core::UnitId id, std::string type, core::Position position);  // occupiesCell = true

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

		// Собирает юнита; билдер после этого пуст
		core::Unit build();

		// Заявка на спавн собранного юнита
		core::SpawnRequest spawnRequest();

		// Заявка на спавн с ошибками фабрики: id, позиция и занятость клетки известны и без
		// юнита, поэтому SetupContext проверит и их (обе ошибки — в одном списке)
		core::SpawnRequest reject(core::Diagnostics errors) const;

	private:
		core::UnitBlueprint _blueprint;
	};

	// Характеристика должна быть больше нуля — иначе unit.invalid_stat
	void requirePositive(core::Diagnostics& errors, std::string_view name, int64_t value);
}
