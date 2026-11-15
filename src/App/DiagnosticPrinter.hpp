#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace sw::app
{
	// Ошибка сценария с привязкой к файлу: так её видит пользователь (SPECIFICATION.md, п. 10.3)
	struct ScenarioError
	{
		std::optional<size_t> line;	 // нет — ошибка всего сценария (нет CREATE_MAP)
		std::string command;  // пусто — команда не распознана или ошибка без строки
		std::string code;
		std::string message;
		std::optional<uint32_t> relatedUnit;  // юнит, о котором ошибка
		std::optional<size_t> relatedLine;	// строка спавна этого юнита, если известна
	};

	// Печатает ошибки в порядке строк (ошибки одной строки — в порядке обнаружения, ошибки
	// без строки — в конце) и итоговую строку:
	//
	//   error: <file>:<line>: <COMMAND> [<code>]: <message>[ (unit <id> spawned at line <L>)]
	//   error: <file>: [<code>]: <message>
	//   <N> error(s), simulation not started.
	void printScenarioErrors(std::ostream& output, const std::string& file, std::vector<ScenarioError> errors);
}
