#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <optional>

namespace sw::core
{
	struct TargetSpec;

	// Контракт: чистая функция. Получает дистанцию после предыдущего правила, nullopt — запрет.
	class ITargetRule
	{
	public:
		ITargetRule() = default;
		virtual ~ITargetRule() = default;

		ITargetRule(const ITargetRule&) = delete;
		ITargetRule& operator=(const ITargetRule&) = delete;
		ITargetRule(ITargetRule&&) = delete;
		ITargetRule& operator=(ITargetRule&&) = delete;

		virtual std::optional<DistanceRange> adjust(const TargetSpec& incoming, DistanceRange range) const = 0;
	};
}
