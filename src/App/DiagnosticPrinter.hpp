#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace sw::app
{
	struct ScenarioError
	{
		std::optional<size_t> line;
		std::string command;
		std::string code;
		std::string message;
		std::optional<uint32_t> relatedUnit;
		std::optional<size_t> relatedLine;
	};

	void printScenarioErrors(std::ostream& output, const std::string& file, std::vector<ScenarioError> errors);
}
