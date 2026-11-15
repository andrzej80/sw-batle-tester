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
	// Ошибка разбора одного поля: код и сообщение с именем поля
	struct FieldError
	{
		std::string code;
		std::string message;
	};

	// Читает поля DTO команды из строки сценария по одному токену (SPECIFICATION.md, п. 8.1).
	// Тип поля проверяется на этапе компиляции: uint32_t — строгий разбор числа,
	// std::string — токен как есть, другой тип — static_assert. Поэтому новый DTO не может
	// незаметно вернуться к operator>>.
	//
	// После первой ошибки остальные поля не читаются: позиция в строке уже не совпадает
	// с полями, и следующие ошибки были бы последствиями первой.
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
