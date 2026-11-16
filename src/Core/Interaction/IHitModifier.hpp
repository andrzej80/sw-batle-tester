#pragma once

namespace sw::core
{
	struct Hit;

	// Контракт: сам проверяет effect и не делает amount отрицательным.
	// Применяются по возрастанию priority(), при равенстве — в порядке добавления.
	class IHitModifier
	{
	public:
		IHitModifier() = default;
		virtual ~IHitModifier() = default;

		IHitModifier(const IHitModifier&) = delete;
		IHitModifier& operator=(const IHitModifier&) = delete;
		IHitModifier(IHitModifier&&) = delete;
		IHitModifier& operator=(IHitModifier&&) = delete;

		virtual int priority() const = 0;
		virtual void modify(Hit& hit) = 0;
	};
}
