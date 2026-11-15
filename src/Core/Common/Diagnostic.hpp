#pragma once

#include "Types.hpp"

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace sw::core
{
	// Ошибка во входных данных. Без привязки к файлу: Core и Features не знают про
	// сценарий, файл, строку и имя команды добавляет App (SPECIFICATION.md, п. 1.2).
	struct Diagnostic
	{
		Diagnostic(std::string code, std::string message, std::optional<UnitId> relatedUnit = std::nullopt);

		std::string code;					// "unit.duplicate_id": пространство имён + суть
		std::string message;				// "unit id 1 already exists"
		std::optional<UnitId> relatedUnit;	// App добавит строку спавна этого юнита

		friend bool operator==(const Diagnostic&, const Diagnostic&) = default;
	};

	std::ostream& operator<<(std::ostream& stream, const Diagnostic& diagnostic);

	// Список ошибок. Проверки собирают все найденные ошибки, а не останавливаются на первой.
	class Diagnostics
	{
	public:
		Diagnostics() = default;
		Diagnostics(std::initializer_list<Diagnostic> diagnostics);

		void add(Diagnostic diagnostic);
		void add(std::string code, std::string message, std::optional<UnitId> relatedUnit = std::nullopt);
		void append(const Diagnostics& other);

		bool empty() const
		{
			return _items.empty();
		}

		size_t size() const
		{
			return _items.size();
		}

		const Diagnostic& operator[](size_t index) const
		{
			return _items.at(index);
		}

		auto begin() const
		{
			return _items.begin();
		}

		auto end() const
		{
			return _items.end();
		}

		friend bool operator==(const Diagnostics&, const Diagnostics&) = default;

	private:
		std::vector<Diagnostic> _items;
	};
}
