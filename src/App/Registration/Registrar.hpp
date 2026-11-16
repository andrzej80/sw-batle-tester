#pragma once

#include <vector>

namespace sw::app
{
	class CommandRegistry;
	class EventBridge;

	// App собирается как OBJECT-библиотека: из STATIC линкер выбросил бы файлы,
	// на которые никто не ссылается, и регистрации пропали бы.
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
		[[maybe_unused]]                                                                                               \
		const ::sw::app::BindingRegistrar SW_APP_CONCAT(swBindingRegistrar_, __LINE__){&(fn)};                         \
	}

#define SW_REGISTER_OUTPUT(fn)                                                                                         \
	namespace                                                                                                          \
	{                                                                                                                  \
		[[maybe_unused]]                                                                                               \
		const ::sw::app::OutputRegistrar SW_APP_CONCAT(swOutputRegistrar_, __LINE__){&(fn)};                           \
	}
