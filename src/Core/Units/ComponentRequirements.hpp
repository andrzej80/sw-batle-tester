#pragma once

#include "ComponentStore.hpp"

#include <Core/Common/TypeName.hpp>
#include <string>
#include <typeindex>
#include <vector>

namespace sw::core
{
	// Компоненты, которые действие читает у своего юнита (IAction::requirements).
	// Проверяются при создании юнита, чтобы ошибка сборки была видна сразу, а не на
	// каком-нибудь 500-м тике (ARCHITECTURE.md, п. 4.3).
	class ComponentRequirements
	{
	public:
		template <class T>
		void require()
		{
			require(typeid(T), typeName<T>());
		}

		void require(std::type_index type, std::string name);

		// Имена компонентов, которых нет в components, в порядке объявления
		std::vector<std::string> missingIn(const ComponentStore& components) const;

	private:
		struct Requirement
		{
			std::type_index type;
			std::string name;
		};

		std::vector<Requirement> _requirements;
	};
}
