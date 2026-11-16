#pragma once

#include "ComponentStore.hpp"

#include <Core/Common/TypeName.hpp>
#include <string>
#include <typeindex>
#include <vector>

namespace sw::core
{
	class ComponentRequirements
	{
	public:
		template <class T>
		void require()
		{
			require(typeid(T), typeName<T>());
		}

		void require(std::type_index type, std::string name);

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
