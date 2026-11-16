#include "CommandParser.hpp"

namespace sw::io
{
	std::vector<ParseDiagnostic> CommandParser::parse(std::istream& stream) const
	{
		std::vector<ParseDiagnostic> diagnostics;
		std::string text;
		size_t lineNumber = 0;
		while (std::getline(stream, text))
		{
			++lineNumber;
			if (text.rfind("//", 0) == 0 || text.empty())
			{
				continue;
			}

			std::istringstream line(text);
			std::string name;
			line >> name;
			if (name.empty())
			{
				continue;
			}

			const auto command = _commands.find(name);
			if (command == _commands.end())
			{
				diagnostics.push_back({lineNumber, "", "parse.unknown_command", "unknown command '" + name + "'"});
				continue;
			}
			if (const std::optional<FieldError> error = command->second(line, lineNumber))
			{
				diagnostics.push_back({lineNumber, name, error->code, error->message});
			}
		}
		return diagnostics;
	}
}
