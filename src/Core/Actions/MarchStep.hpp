#pragma once

#include "IAction.hpp"

namespace sw::core
{
	// За ход — до Movement::speed шагов к цели активного марша.
	class MarchStep final : public IAction
	{
	public:
		bool tryExecute(const Unit& self, TurnContext& context) override;
		void requirements(ComponentRequirements& requirements) const override;
	};
}
