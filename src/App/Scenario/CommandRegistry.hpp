#pragma once

#include <Core/Common/Result.hpp>
#include <Core/Setup/SetupContext.hpp>
#include <IO/System/CommandParser.hpp>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sw::app
{
	// Разобранная команда сценария: номер строки и её выполнение над SetupContext.
	// Шаг можно выполнить несколько раз — так работают два прохода (проверка и запуск).
	struct ScenarioStep
	{
		size_t line;
		std::string command;
		std::function<core::Result<void>(core::SetupContext&)> execute;
	};

	// Связь «DTO команды → адаптер» (SPECIFICATION.md, п. 10.2). Адаптеры регистрируются сами
	// (SW_REGISTER_BINDING), реестр собирает из них разбор сценария.
	class CommandRegistry
	{
	public:
		template <class TCommand>
		using Handler = std::function<core::Result<void>(const TCommand&, core::SetupContext&)>;

		// Команда уже зарегистрирована или обработчик пустой — std::logic_error (баг сборки)
		template <class TCommand>
		CommandRegistry& on(Handler<TCommand> handler)
		{
			const std::string name = TCommand::Name;
			if (!handler)
			{
				throw std::logic_error("CommandRegistry: empty handler for " + name);
			}
			if (!_names.insert(name).second)
			{
				throw std::logic_error("CommandRegistry: command " + name + " is registered twice");
			}
			_installers.push_back(
				[handler = std::move(handler)](io::CommandParser& parser, std::vector<ScenarioStep>& steps)
				{
					parser.add<TCommand>(
						[handler, &steps](TCommand command, size_t line)
						{
							steps.push_back(
								{line,
								 TCommand::Name,
								 [handler, command = std::move(command)](core::SetupContext& context)
								 {
									 return handler(command, context);
								 }});
						});
				});
			return *this;
		}

		// Регистрирует все команды в parser; разобранные шаги складываются в steps
		void installInto(io::CommandParser& parser, std::vector<ScenarioStep>& steps) const;

	private:
		using Installer = std::function<void(io::CommandParser&, std::vector<ScenarioStep>&)>;

		std::vector<Installer> _installers;
		std::unordered_set<std::string> _names;
	};
}
