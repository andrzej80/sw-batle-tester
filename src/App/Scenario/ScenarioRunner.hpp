#pragma once

#include "ScenarioLoader.hpp"

#include <App/Registration/Registrar.hpp>
#include <Core/Common/Types.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <vector>

namespace sw::core
{
	class IRandom;
}

namespace sw::app
{
	// Проход 2 — запуск (SPECIFICATION.md, п. 6.1, 10.2): новый мир, к шине подключены
	// адаптеры вывода, шаги выполняются заново (события тика 1), затем симуляция.
	// Вызывается только после прохода 1 без ошибок: ошибка шага здесь — std::logic_error,
	// потому что проверка её пропустила.
	core::StopReason runScenario(
		const LoadedScenario& scenario,
		const std::vector<OutputFn>& outputs,
		core::IRandom& random,
		core::Tick tickLimit);
}
