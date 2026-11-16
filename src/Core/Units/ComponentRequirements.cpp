#include "ComponentRequirements.hpp"

#include <algorithm>
#include <utility>

namespace sw::core
{
	void ComponentRequirements::require(std::type_index type, std::string name)
	{
		const bool known = std::any_of(
			_requirements.begin(),
			_requirements.end(),
			[type](const Requirement& requirement) { return requirement.type == type; });
		if (!known)
		{
			_requirements.push_back({type, std::move(name)});
		}
	}

	std::vector<std::string> ComponentRequirements::missingIn(const ComponentStore& components) const
	{
		std::vector<std::string> missing;
		for (const Requirement& requirement : _requirements)
		{
			if (!components.contains(requirement.type))
			{
				missing.push_back(requirement.name);
			}
		}
		return missing;
	}
}
