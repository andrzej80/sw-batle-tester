#pragma once

#include "ComponentStore.hpp"

#include <Core/Actions/IAction.hpp>
#include <Core/Common/Tag.hpp>
#include <Core/Common/Types.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sw::core
{
	// Всё, из чего собирается юнит. Заполняет UnitBuilder в Features, юнита из него
	// создаёт Core.
	struct UnitBlueprint
	{
		UnitId id;
		std::string type;  // "Swordsman"; выводится в UNIT_SPAWNED
		Position position;
		bool occupiesCell = true;
		TagSet tags;
		ComponentStore components;
		std::vector<std::unique_ptr<IAction>> behaviour;  // по приоритету
	};

	// Юнит (ARCHITECTURE.md, п. 4.1; SPECIFICATION.md, п. 3.1).
	//
	// Фичи получают юнита только как const Unit&: читать можно всё, а менять мир —
	// только через фасады TurnContext. Изменяющие методы ниже вызывает Core:
	// позицию — MovementService, Health — Interaction, марши — SetupContext и MarchStep.
	class Unit
	{
	public:
		// Проверяет требования всех действий к компонентам. Недостающие — std::logic_error
		// со списком «действие → компонент»: это баг сборки юнита, а не ошибка ввода.
		explicit Unit(UnitBlueprint blueprint);

		UnitId id() const
		{
			return _id;
		}

		std::string_view type() const
		{
			return _type;
		}

		Position position() const
		{
			return _position;
		}

		bool occupiesCell() const
		{
			return _occupiesCell;
		}

		const TagSet& tags() const
		{
			return _tags;
		}

		// HP ≤ 0. Юнит без Health не умирает — его можно только удалить (Мина)
		bool isDead() const;

		bool isRemovalRequested() const
		{
			return _removalRequested;
		}

		template <class T>
		bool has() const
		{
			return _components.has<T>();
		}

		template <class T>
		const T& get() const
		{
			return _components.get<T>();
		}

		template <class T>
		T& get()
		{
			return _components.get<T>();
		}

		template <class T>
		const T* tryGet() const
		{
			return _components.tryGet<T>();
		}

		template <class T>
		T* tryGet()
		{
			return _components.tryGet<T>();
		}

		const ComponentStore& components() const
		{
			return _components;
		}

		ComponentStore& components()
		{
			return _components;
		}

		// Только для Simulation: действия вызываются с изменяемым состоянием
		std::vector<std::unique_ptr<IAction>>& behaviour()
		{
			return _behaviour;
		}

		void setPosition(Position position)
		{
			_position = position;
		}

		void requestRemoval()
		{
			_removalRequested = true;
		}

	private:
		UnitId _id;
		std::string _type;
		Position _position;
		bool _occupiesCell;
		TagSet _tags;
		ComponentStore _components;
		std::vector<std::unique_ptr<IAction>> _behaviour;
		bool _removalRequested = false;
	};
}
