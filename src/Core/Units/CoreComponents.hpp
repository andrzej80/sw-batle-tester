#pragma once

#include "IComponent.hpp"

#include <Core/Common/Types.hpp>
#include <cstdint>
#include <deque>
#include <optional>

namespace sw::core
{
	class Health final : public IComponent
	{
	public:
		explicit Health(int64_t initial);

		// Может стать меньше 0 — это смерть; в событиях выводится max(0, current)
		int64_t current;
		int64_t max;
	};

	class Movement final : public IComponent
	{
	public:
		explicit Movement(int64_t speed);

		int64_t speed;
		// Первая цель — активный марш
		std::deque<Position> marches;
	};

	enum class Reapply : std::uint8_t
	{
		Replace,
		Keep,
	};

	class Status : public IComponent
	{
	public:
		explicit Status(std::optional<Tick> expiresAt);

		std::optional<Tick> expiresAt() const
		{
			return _expiresAt;
		}

		bool isExpired(Tick now) const
		{
			return _expiresAt.has_value() && *_expiresAt <= now;
		}

		// По умолчанию новый статус заменяет старый. Keep — оставить этот (продлив срок или
		// сложив стаки в переопределении).
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
