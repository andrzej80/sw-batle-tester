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
	struct ScenarioStep
	{
		size_t line;
		std::string command;
		std::function<core::Result<void>(core::SetupContext&)> execute;
	};

	class CommandRegistry
	{
	public:
		template <class TCommand>
		using Handler = std::function<core::Result<void>(const TCommand&, core::SetupContext&)>;

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

		void installInto(io::CommandParser& parser, std::vector<ScenarioStep>& steps) const;

	private:
		using Installer = std::function<void(io::CommandParser&, std::vector<ScenarioStep>&)>;

		std::vector<Installer> _installers;
		std::unordered_set<std::string> _names;
	};
}
