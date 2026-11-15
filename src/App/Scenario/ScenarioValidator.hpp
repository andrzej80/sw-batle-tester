#pragma once

#include "ScenarioLoader.hpp"

#include <App/DiagnosticPrinter.hpp>
#include <vector>

namespace sw::app
{
	// Проход 1 — проверка сценария (SPECIFICATION.md, п. 6.1, 10.2). Выполняет все шаги на
	// отдельном мире с шиной без подписчиков (в stdout ничего не попадает) и собирает
	// полный список ошибок: ошибки разбора и ошибки выполнения всех строк, затем ошибки
	// сценария целиком (нет CREATE_MAP). К ошибке о юните добавляется строка его спавна.
	// Пустой список — сценарий можно запускать.
	std::vector<ScenarioError> validateScenario(const LoadedScenario& scenario);
}
