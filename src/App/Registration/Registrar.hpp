#pragma once

#include <vector>

namespace sw::app
{
	class CommandRegistry;
	class EventBridge;

	// Саморегистрация адаптеров (ARCHITECTURE.md, п. 5.4): общих списков, которые правят все
	// разработчики, нет. Адаптер в конце своего файла пишет
	//
	//   SW_REGISTER_BINDING(bindHunter);   // команда сценария → фича
	//   SW_REGISTER_OUTPUT(outputUnitHealed);  // событие → формат вывода
	//
	// Списки — статические переменные внутри функций: создаются при первом обращении, поэтому
	// порядок инициализации статиков между файлами не важен. Глобально хранятся только
	// указатели на функции: CommandRegistry и EventBridge Application создаёт сам.
	//
	// ВАЖНО: App собирается как OBJECT-библиотека (CMakeLists.txt): из статической библиотеки
	// линкер выбросил бы файлы, на которые никто не ссылается, и регистрации пропали бы.
	using BindingFn = void (*)(CommandRegistry&);
	using OutputFn = void (*)(EventBridge&);

	std::vector<BindingFn>& bindingList();
	std::vector<OutputFn>& outputList();

	struct BindingRegistrar
	{
		explicit BindingRegistrar(BindingFn bind);
	};

	struct OutputRegistrar
	{
		explicit OutputRegistrar(OutputFn bind);
	};
}

#define SW_APP_CONCAT_IMPL(a, b) a##b
#define SW_APP_CONCAT(a, b) SW_APP_CONCAT_IMPL(a, b)

#define SW_REGISTER_BINDING(fn)                                                                                        \
	namespace                                                                                                          \
	{                                                                                                                  \
		const ::sw::app::BindingRegistrar SW_APP_CONCAT(swBindingRegistrar_, __LINE__){&(fn)};                         \
	}

#define SW_REGISTER_OUTPUT(fn)                                                                                         \
	namespace                                                                                                          \
	{                                                                                                                  \
		const ::sw::app::OutputRegistrar SW_APP_CONCAT(swOutputRegistrar_, __LINE__){&(fn)};                           \
	}
