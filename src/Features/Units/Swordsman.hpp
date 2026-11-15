#pragma once

#include <Core/Common/Types.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <cstdint>

namespace sw::features
{
	// Мечник (README задания): HP, Strength.
	// Сокрушающий удар — случайный юнит в соседней клетке, урон Strength; если бить
	// некого — шаг марша. Ничего не знает про формат ввода: его заполняет App/Bindings.
	struct SwordsmanSpec
	{
		core::UnitId id;
		core::Position position;
		int64_t hp;
		int64_t strength;
	};

	// Проверяет hp > 0 и strength > 0 (unit.invalid_stat) и возвращает все ошибки сразу.
	// Заявка несёт id, позицию и занятость клетки и тогда, когда юнит не собрался.
	core::SpawnRequest makeSwordsman(const SwordsmanSpec& spec);
}
