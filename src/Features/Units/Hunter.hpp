#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <cstdint>

namespace sw::features
{
	// Охотник (README задания): HP, Agility, Strength, Range.
	// - Стремительный выстрел — случайный юнит на расстоянии 2..Range, урон Agility;
	//   только если в соседних клетках нет юнитов, занимающих клетку.
	// - Удар из тени — если стрелять не может: случайный сосед, урон Strength.
	// - Если бить некого — шаг марша.
	struct HunterSpec
	{
		core::UnitId id;
		core::Position position;
		int64_t hp;
		int64_t agility;
		int64_t strength;
		int64_t range;
	};

	// Проверяет hp, agility, strength > 0 (unit.invalid_stat) и range ≥ 2
	// (hunter.invalid_range: с меньшей дальностью Охотник никогда не выстрелит);
	// возвращает все ошибки сразу.
	core::SpawnRequest makeHunter(const HunterSpec& spec);
}
