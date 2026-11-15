#pragma once

#include <Core/Common/DistanceRange.hpp>
#include <Core/Common/Tag.hpp>
#include <Core/Common/Types.hpp>
#include <cstdint>
#include <utility>

namespace sw::core
{
	// Против цели или в её пользу. Ось не растёт: на ней держатся правила вроде
	// «ближняя атака по Ворону запрещена» (ARCHITECTURE.md, п. 4.9).
	enum class Intent : std::uint8_t
	{
		Hostile,
		Beneficial,
	};

	// Что меняется у цели. Новая операция (сбивание баффа, отбрасывание) — новое значение.
	enum class Effect : std::uint8_t
	{
		Damage,
		Heal,
		Status,
	};

	// Описание воздействия: по нему отбираются цели и применяются правила и модификаторы.
	// Конструктора по умолчанию нет: автор действия обязан выбрать и Intent, и Effect.
	// В Features есть именованные конструкторы attack / heal / buff / debuff.
	struct TargetSpec
	{
		TargetSpec(Intent intent, Effect effect, TagSet tags, DistanceRange range) :
				intent(intent),
				effect(effect),
				tags(std::move(tags)),
				range(range)
		{}

		Intent intent;
		Effect effect;
		TagSet tags;  // способ доставки (melee, ranged, area) + тип воздействия (physical, fire…)
		DistanceRange range;
	};

	// Удар или лечение одной цели. Intent и Effect берутся из spec, отдельного поля
	// «вид удара» нет, поэтому две записи одного и того же не разойдутся.
	struct Hit
	{
		TargetSpec spec;
		UnitId source;
		UnitId target;
		int64_t amount;	 // ≥ 0 на входе и после каждого модификатора
	};
}
