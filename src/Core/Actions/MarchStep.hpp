#pragma once

#include "IAction.hpp"

namespace sw::core
{
	// Первый шаг за ход разрешён при любой стоимости клетки, иначе дорогая клетка
	// заблокировала бы юнита навсегда.
	class MarchStep final : public IAction
	{
	public:
		bool tryExecute(const Unit& self, TurnContext& context) override;
		void requirements(ComponentRequirements& requirements) const override;
	};
}
