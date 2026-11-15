#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <optional>

namespace sw::core
{
	struct TargetSpec;

	// Правило цели: компонент или статус ЦЕЛИ решает, можно ли выбрать её этим
	// воздействием и с какой дистанции (ARCHITECTURE.md, п. 4.9). Поэтому новый иммунитет
	// добавляется в цель, а код атакующих не меняется.
	//
	// Контракт (п. 4.13): чистая функция без побочных эффектов; явно проверяет нужную ось
	// (правило про атаки — intent == Hostile). Правила применяются по очереди в порядке
	// добавления, каждое получает дистанцию после предыдущего; nullopt — запрет.
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
