#pragma once

#include "details/CommandParserVisitor.hpp"

#include <cstddef>
#include <functional>
#include <istream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sw::io
{
	struct ParseDiagnostic
	{
		size_t line;
		std::string command;
		std::string code;
		std::string message;
	};

	class CommandParser
	{
	public:
		template <class TCommand>
		CommandParser& add(std::function<void(TCommand, size_t line)> handler)
		{
			if (!handler)
			{
				throw std::logic_error(std::string("CommandParser: empty handler for ") + TCommand::Name);
			}
			const std::string name = TCommand::Name;
			auto [it, inserted] = _commands.emplace(
				name,
				[handler = std::move(handler)](std::istream& stream, size_t line) -> std::optional<FieldError>
				{
					TCommand command;
					CommandParserVisitor visitor(stream);
					command.visit(visitor);
					if (visitor.error())
					{
						return visitor.error();
					}
					std::string extra;
					if (stream >> extra)
					{
						return FieldError{"parse.extra_argument", "unexpected extra argument '" + extra + "'"};
					}
					handler(std::move(command), line);
					return std::nullopt;
				});
			if (!inserted)
			{
				throw std::logic_error("CommandParser: command already exists: " + name);
			}
			return *this;
		}

		std::vector<ParseDiagnostic> parse(std::istream& stream) const;

	private:
		using Reader = std::function<std::optional<FieldError>(std::istream&, size_t)>;

		std::unordered_map<std::string, Reader> _commands;
	};
}
