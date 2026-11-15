#pragma once

#include "Registration/Registrar.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace sw::app
{
	// Коды выхода (SPECIFICATION.md, п. 10.1)
	namespace exit_code
	{
		inline constexpr int Success = 0;	   // в том числе остановка по лимиту тиков
		inline constexpr int Environment = 1;  // нет аргумента, файл не открылся
		inline constexpr int InvalidScenario = 2;  // ошибки сценария — полный список в stderr
		inline constexpr int InternalError = 3;	 // нарушенный инвариант или контракт (баг)
	}

	// Точка входа приложения: аргументы командной строки, загрузка и проверка сценария,
	// запуск симуляции, коды выхода. Сообщения — в errors (stderr), события — в stdout.
	class Application
	{
	public:
		// Адаптеры команд; по умолчанию — все саморегистрированные (SW_REGISTER_BINDING)
		explicit Application(std::vector<BindingFn> bindings = bindingList());

		// Не бросает: исключение — это баг, он печатается как internal error с кодом 3
		int run(int argc, const char* const* argv, std::ostream& errors) const;

	private:
		int runScenario(const std::string& path, std::ostream& errors) const;

		std::vector<BindingFn> _bindings;
	};
}
