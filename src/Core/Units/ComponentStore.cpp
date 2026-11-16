#include "ComponentStore.hpp"

#include "CoreComponents.hpp"

#include <Core/Interaction/ITargetRule.hpp>
#include <algorithm>

namespace sw::core
{
	namespace
	{
		template <class T>
		void erase(std::vector<T>& items, const T& item)
		{
			items.erase(std::remove(items.begin(), items.end(), item), items.end());
		}
	}

	void ComponentStore::add(std::type_index type, std::unique_ptr<IComponent> component)
	{
		if (!component)
		{
			throw std::logic_error("ComponentStore: null component");
		}
		if (_components.contains(type))
		{
			throw std::logic_error("ComponentStore: component " + typeName(*component) + " is already present");
		}
		if (const auto* rule = dynamic_cast<const ITargetRule*>(component.get()))
		{
			_targetRules.push_back(rule);
		}
		if (const auto* status = dynamic_cast<const Status*>(component.get()))
		{
			_statuses.push_back(status);
		}
		_components.emplace(type, std::move(component));
	}

	bool ComponentStore::remove(std::type_index type)
	{
		const auto found = _components.find(type);
		if (found == _components.end())
		{
			return false;
		}
		IComponent* component = found->second.get();
		if (const auto* rule = dynamic_cast<const ITargetRule*>(component))
		{
			erase(_targetRules, rule);
		}
		if (const auto* status = dynamic_cast<const Status*>(component))
		{
			erase(_statuses, status);
		}
		_components.erase(found);
		return true;
	}
}
