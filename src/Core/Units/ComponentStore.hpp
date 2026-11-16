#pragma once

#include "IComponent.hpp"

#include <Core/Common/TypeName.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sw::core
{
	class ITargetRule;
	class IHitModifier;

	class ComponentStore
	{
	public:
		template <class T, class... Args>
		T& add(Args&&... args)
		{
			static_assert(std::is_base_of_v<IComponent, T>, "component must derive from IComponent");
			auto component = std::make_unique<T>(std::forward<Args>(args)...);
			T& reference = *component;
			add(typeid(T), std::move(component));
			return reference;
		}

		void add(std::type_index type, std::unique_ptr<IComponent> component);

		bool remove(std::type_index type);

		size_t removeIf(const std::function<bool(const IComponent&)>& predicate);

		template <class T>
		bool remove()
		{
			return remove(typeid(T));
		}

		template <class T>
		bool has() const
		{
			return _components.contains(typeid(T));
		}

		template <class T>
		const T* tryGet() const
		{
			const auto found = _components.find(typeid(T));
			return found == _components.end() ? nullptr : static_cast<const T*>(found->second.get());
		}

		template <class T>
		T* tryGet()
		{
			return const_cast<T*>(std::as_const(*this).tryGet<T>());
		}

		template <class T>
		const T& get() const
		{
			const T* component = tryGet<T>();
			if (component == nullptr)
			{
				throw std::logic_error("component " + typeName<T>() + " is missing");
			}
			return *component;
		}

		template <class T>
		T& get()
		{
			return const_cast<T&>(std::as_const(*this).get<T>());
		}

		bool contains(std::type_index type) const
		{
			return _components.contains(type);
		}

		IComponent* find(std::type_index type)
		{
			const auto found = _components.find(type);
			return found == _components.end() ? nullptr : found->second.get();
		}

		const std::vector<const ITargetRule*>& targetRules() const
		{
			return _targetRules;
		}

		const std::vector<IHitModifier*>& hitModifiers() const
		{
			return _hitModifiers;
		}

	private:
		std::unordered_map<std::type_index, std::unique_ptr<IComponent>> _components;
		std::vector<const ITargetRule*> _targetRules;
		std::vector<IHitModifier*> _hitModifiers;
	};
}
