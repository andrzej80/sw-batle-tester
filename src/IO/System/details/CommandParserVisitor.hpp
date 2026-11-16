#pragma once

#include "../ParseNumber.hpp"

#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace sw::io
{
	struct FieldError
	{
		std::string code;
		std::string message;
	};

	// После первой ошибки поля не читаются: следующие ошибки были бы её последствиями.
	class CommandParserVisitor
	{
	public:
		explicit CommandParserVisitor(std::istream& stream) :
				_stream(stream)
		{}

		template <class TField>
		void visit(const char* name, TField& field)
		{
			static_assert(
				std::is_same_v<TField, uint32_t> || std::is_same_v<TField, std::string>,
				"command fields must be uint32_t or std::string: add strict parsing for the new type");
			if (_error)
			{
				return;
			}

			std::string token;
			if (!(_stream >> token))
			{
				_error = FieldError{"parse.missing_argument", std::string("missing argument '") + name + "'"};
				return;
			}

			if constexpr (std::is_same_v<TField, uint32_t>)
			{
				std::string problem;
				if (const std::optional<uint32_t> value = parseUInt32(token, problem))
				{
					field = *value;
				}
				else
				{
					_error = FieldError{"parse.invalid_number", std::string(name) + ": " + problem};
				}
			}
			else
			{
				field = std::move(token);
			}
		}

		const std::optional<FieldError>& error() const
		{
			return _error;
		}

	private:
		std::istream& _stream;
		std::optional<FieldError> _error;
	};
}
