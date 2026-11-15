#pragma once

namespace sw::core
{
	// Компонент юнита: характеристика (Health, Strength), способность (Movement),
	// свойство или статус. Хранится в ComponentStore по типу, по одному каждого типа.
	// Компонент может дополнительно реализовать ITargetRule и/или IHitModifier —
	// тогда он попадает в индекс хуков и участвует в отборе целей и применении эффектов.
	class IComponent
	{
	public:
		IComponent() = default;
		virtual ~IComponent() = default;

		IComponent(const IComponent&) = delete;
		IComponent& operator=(const IComponent&) = delete;
		IComponent(IComponent&&) = delete;
		IComponent& operator=(IComponent&&) = delete;
	};
}
