#pragma once

#include "Diagnostic.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace sw::core
{
	namespace detail
	{
		inline Diagnostics requireErrors(Diagnostics errors)
		{
			if (errors.empty())
			{
				throw std::logic_error("Result: an error result requires at least one diagnostic");
			}
			return errors;
		}
	}

	template <class T>
	class [[nodiscard]] Result
	{
	public:
		Result(T value) :
				_state(std::in_place_index<0>, std::move(value))
		{}

		Result(Diagnostics errors) :
				_state(std::in_place_index<1>, detail::requireErrors(std::move(errors)))
		{}

		Result(Diagnostic error) :
				Result(Diagnostics{std::move(error)})
		{}

		bool ok() const
		{
			return _state.index() == 0;
		}

		const T& value() const&
		{
			requireValue();
			return std::get<0>(_state);
		}

		T& value() &
		{
			requireValue();
			return std::get<0>(_state);
		}

		T&& value() &&
		{
			requireValue();
			return std::get<0>(std::move(_state));
		}

		const Diagnostics& errors() const&
		{
			static const Diagnostics noErrors;
			return ok() ? noErrors : std::get<1>(_state);
		}

		// По значению: иначе for (const auto& e : make().errors()) обходил бы висячую ссылку
		Diagnostics errors() &&
		{
			return ok() ? Diagnostics{} : std::get<1>(std::move(_state));
		}

	private:
		void requireValue() const
		{
			if (!ok())
			{
				throw std::logic_error("Result: value() called on a result with errors");
			}
		}

		std::variant<T, Diagnostics> _state;
	};

	template <>
	class [[nodiscard]] Result<void>
	{
	public:
		Result() = default;

		Result(Diagnostics errors) :
				_errors(detail::requireErrors(std::move(errors)))
		{}

		Result(Diagnostic error) :
				Result(Diagnostics{std::move(error)})
		{}

		bool ok() const
		{
			return _errors.empty();
		}

		const Diagnostics& errors() const&
		{
			return _errors;
		}

		Diagnostics errors() &&
		{
			return std::move(_errors);
		}

	private:
		Diagnostics _errors;
	};
}
