#pragma once

namespace sw::core
{
	struct Hit;

	// Модификатор удара: компонент или статус ЦЕЛИ меняет величину урона или лечения
	// (щит, резист, запрет лечения) — ARCHITECTURE.md, п. 4.9.
	//
	// Контракт (п. 4.13): явно проверяет effect (резист к magic уменьшает только Damage);
	// amount не становится отрицательным; меняет только Hit и своё состояние (остаток щита).
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
