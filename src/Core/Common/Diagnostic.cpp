#include "Diagnostic.hpp"

#include <ostream>
#include <utility>

namespace sw::core
{
	Diagnostic::Diagnostic(std::string code, std::string message, std::optional<UnitId> relatedUnit) :
			code(std::move(code)),
			message(std::move(message)),
			relatedUnit(relatedUnit)
	{}

	std::ostream& operator<<(std::ostream& stream, const Diagnostic& diagnostic)
	{
		stream << '[' << diagnostic.code << "]: " << diagnostic.message;
		if (diagnostic.relatedUnit)
		{
			stream << " (related unit " << *diagnostic.relatedUnit << ')';
		}
		return stream;
	}

	Diagnostics::Diagnostics(std::initializer_list<Diagnostic> diagnostics) :
			_items(diagnostics)
	{}

	void Diagnostics::add(Diagnostic diagnostic)
	{
		_items.push_back(std::move(diagnostic));
	}

	void Diagnostics::add(std::string code, std::string message, std::optional<UnitId> relatedUnit)
	{
		_items.push_back({std::move(code), std::move(message), relatedUnit});
	}

	void Diagnostics::append(const Diagnostics& other)
	{
		_items.insert(_items.end(), other._items.begin(), other._items.end());
	}
}
