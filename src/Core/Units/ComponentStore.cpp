#include "ComponentStore.hpp"

#include <Core/Interaction/IHitModifier.hpp>
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
		if (auto* modifier = dynamic_cast<IHitModifier*>(component.get()))
		{
			_hitModifiers.push_back(modifier);
		}
		_components.emplace(type, std::move(component));
	}

	size_t ComponentStore::removeIf(const std::function<bool(const IComponent&)>& predicate)
	{
		std::vector<std::type_index> doomed;
		for (const auto& [type, component] : _components)
		{
			if (predicate(*component))
			{
				doomed.push_back(type);
			}
		}
		for (const std::type_index type : doomed)
		{
			remove(type);
		}
		return doomed.size();
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
		if (auto* modifier = dynamic_cast<IHitModifier*>(component))
		{
			erase(_hitModifiers, modifier);
		}
		_components.erase(found);
		return true;
	}
}
