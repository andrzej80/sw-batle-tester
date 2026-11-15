#pragma once

namespace sw::app
{
	// Точка входа приложения: аргументы командной строки, загрузка и проверка
	// сценария, запуск симуляции, коды выхода (SPECIFICATION.md, п. 10.1).
	class Application
	{
	public:
		int run(int argc, char** argv);
	};
}
