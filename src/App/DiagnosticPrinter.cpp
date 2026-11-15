#include "DiagnosticPrinter.hpp"

#include <algorithm>
#include <limits>
#include <ostream>

namespace sw::app
{
	void printScenarioErrors(std::ostream& output, const std::string& file, std::vector<ScenarioError> errors)
	{
		std::stable_sort(
			errors.begin(),
			errors.end(),
			[](const ScenarioError& a, const ScenarioError& b)
			{
				constexpr size_t noLine = std::numeric_limits<size_t>::max();
				return a.line.value_or(noLine) < b.line.value_or(noLine);
			});

		for (const ScenarioError& error : errors)
		{
			output << "error: " << file;
			if (error.line)
			{
				output << ':' << *error.line;
			}
			output << ": ";
			if (!error.command.empty())
			{
				output << error.command << ' ';
			}
			output << '[' << error.code << "]: " << error.message;
			if (error.relatedUnit && error.relatedLine)
			{
				output << " (unit " << *error.relatedUnit << " spawned at line " << *error.relatedLine << ')';
			}
			output << '\n';
		}
		output << errors.size() << (errors.size() == 1 ? " error" : " errors") << ", simulation not started.\n";
	}
}
