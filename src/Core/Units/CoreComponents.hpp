#pragma once

#include "IComponent.hpp"

#include <Core/Common/Types.hpp>
#include <cstdint>
#include <deque>
#include <optional>

// Компоненты Core (ARCHITECTURE.md, п. 4.1). Все необязательны: без Health юнит нельзя
// ранить, вылечить и убить (Мина); без Movement юнит статичен при любых условиях (Башня).
namespace sw::core
{
	class Health final : public IComponent
	{
	public:
		// HP при спавне — оно же максимум. HP ≤ 0 при создании — std::logic_error:
		// фабрика юнита обязана отклонить такой спавн ошибкой ввода раньше.
		explicit Health(int64_t initial);

		// Может быть ниже 0 — это и есть смерть; в событиях выводится как max(0, current)
		int64_t current;
		int64_t max;
	};

	class Movement final : public IComponent
	{
	public:
		// Клеток за ход; speed < 1 — std::logic_error
		explicit Movement(int64_t speed);

		int64_t speed;
		std::deque<Position> marches;  // очередь целей; первая — активный марш
	};

	// Что делать при повторном наложении статуса того же типа
	enum class Reapply : std::uint8_t
	{
		Replace,  // новый экземпляр заменяет старый
		Keep,  // остаётся старый (он мог обновить себя в onReapplied), новый отбрасывается
	};

	// База статусов: компонент со сроком жизни (горение, щит). Истёкший статус снимается
	// в фазе Cleanup. Конкретные статусы — в Features.
	class Status : public IComponent
	{
	public:
		explicit Status(std::optional<Tick> expiresAt);	 // nullopt — бессрочный

		std::optional<Tick> expiresAt() const
		{
			return _expiresAt;
		}

		// Истёк к концу тика now
		bool isExpired(Tick now) const
		{
			return _expiresAt.has_value() && *_expiresAt <= now;
		}

		// Статус того же типа наложили повторно. Решает сам статус: по умолчанию новый
		// заменяет старый (обновляются срок и параметры). Переопределение может продлить
		// срок или сложить стаки — изменить себя и вернуть Keep.
		virtual Reapply onReapplied(const Status& /*incoming*/)
		{
			return Reapply::Replace;
		}

	protected:
		void setExpiresAt(std::optional<Tick> expiresAt)
		{
			_expiresAt = expiresAt;
		}

	private:
		std::optional<Tick> _expiresAt;
	};
}
