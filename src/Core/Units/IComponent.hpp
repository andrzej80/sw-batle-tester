#pragma once

namespace sw::core
{
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
