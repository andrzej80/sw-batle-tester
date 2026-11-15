#pragma once

#include "IAction.hpp"

namespace sw::core
{
	// Шаг активного марша (ARCHITECTURE.md, п. 4.7; SPECIFICATION.md, п. 6.3). Обычное
	// действие: фича ставит его на любое место Behaviour, порядок «бить или идти» в Core
	// не зашит.
	//
	// За ход юнит тратит бюджет Movement::speed:
	// - кандидаты — соседние клетки, уменьшающие расстояние до цели и свободные для юнита;
	// - сначала прямой шаг (sign(dx), sign(dy)), если он среди кандидатов, иначе случайный
	//   из кандидатов (обход);
	// - стоимость входа даёт клетка; первый шаг за ход разрешён всегда, чтобы дорогая
	//   клетка не заблокировала юнита навсегда;
	// - движение за ход заканчивается на приходе в цель: MarchEnded, следующий марш
	//   стартует в том же тике, а идти начинает со следующего.
	// Кандидатов нет — false: юнит стоит, марш не отменяется.
	class MarchStep final : public IAction
	{
	public:
		bool tryExecute(const Unit& self, TurnContext& context) override;
		void requirements(ComponentRequirements& requirements) const override;
	};
}
