#pragma once

#include <Core/Common/Tag.hpp>

// Теги, которыми пользуются почти все фичи (ARCHITECTURE.md, п. 4.9). Меняются редко.
// Тег конкретной механики (fire, explosion, heal) объявляется в файле своей фичи:
// одинаковое имя в разных фичах — один и тот же тег.
namespace sw::features::tags
{
	// Способ доставки
	inline const core::Tag Melee{"melee"};
	inline const core::Tag Ranged{"ranged"};
	inline const core::Tag Area{"area"};

	// Тип воздействия
	inline const core::Tag Physical{"physical"};

	// Свойство юнита
	inline const core::Tag Flying{"flying"};
}
