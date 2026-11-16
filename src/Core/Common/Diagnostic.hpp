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
	struct Diagnostic
	{
		Diagnostic(std::string code, std::string message, std::optional<UnitId> relatedUnit = std::nullopt);

		std::string code;
		std::string message;
		std::optional<UnitId> relatedUnit;

		friend bool operator==(const Diagnostic&, const Diagnostic&) = default;
	};

	std::ostream& operator<<(std::ostream& stream, const Diagnostic& diagnostic);

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
