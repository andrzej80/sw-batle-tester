#pragma once

#include "Registration/Registrar.hpp"

#include <Core/Common/Types.hpp>
#include <Core/Random/IRandom.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <functional>
#include <iosfwd>
#include <memory>
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

	// Из чего собирается приложение. По умолчанию — как в main: все саморегистрированные
	// адаптеры, StdRandom с seed от std::random_device, лимит 30 000 тиков. Тесты подставляют
	// свои адаптеры, детерминированную случайность и маленький лимит.
	struct ApplicationOptions
	{
		std::vector<BindingFn> bindings = bindingList();
		std::vector<OutputFn> outputs = outputList();
		std::function<std::unique_ptr<core::IRandom>()> random = []
		{
			return std::make_unique<core::StdRandom>(core::StdRandom::seedFromDevice());
		};
		core::Tick tickLimit = core::Simulation::DefaultTickLimit;
	};

	// Точка входа приложения: аргументы командной строки, загрузка, проверка (проход 1) и
	// запуск (проход 2) сценария, коды выхода. Сообщения — в errors (stderr), события — в stdout.
	class Application
	{
	public:
		explicit Application(ApplicationOptions options = {});

		// Не бросает: исключение — это баг, он печатается как internal error с кодом 3
		int run(int argc, const char* const* argv, std::ostream& errors) const;

	private:
		int processFile(const std::string& path, std::ostream& errors) const;

		ApplicationOptions _options;
	};
}
