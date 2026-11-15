#include "Application.hpp"

#include <iostream>

namespace sw::app
{
	// Заглушка каркаса (задача T01). Полная реализация — задачи T12 и T15.
	// Код выхода 0, чтобы шаг CI «Run example scenario» не падал до появления симуляции.
	int Application::run(int /*argc*/, char** /*argv*/)
	{
		std::cerr << "sw_battle_test: scenario processing is not implemented yet\n";
		return 0;
	}
}
