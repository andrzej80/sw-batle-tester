#pragma once

#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace sw::test
{
	struct SourceLocation
	{
		const char* file;
		int line;
	};

	using TestFunction = void (*)();

	struct TestCase
	{
		std::string name;
		TestFunction function;
		SourceLocation location;
	};

	std::vector<TestCase>& registry();

	struct TestRegistrar
	{
		TestRegistrar(const char* name, TestFunction function, SourceLocation location);
	};

	void reportFailure(SourceLocation location, const std::string& message);

	size_t runIsolated(const std::function<void()>& body, std::ostream& output);

	int runAll(std::string_view filter, std::ostream& output);

	namespace detail
	{
		template <class T>
		std::string describe(const T& value)
		{
			if constexpr (requires(std::ostream& stream, const T& v) { stream << v; })
			{
				std::ostringstream stream;
				stream << value;
				return stream.str();
			}
			else
			{
				return "<unprintable>";
			}
		}
	}
}

#define SW_TEST_CONCAT_IMPL(a, b) a##b
#define SW_TEST_CONCAT(a, b) SW_TEST_CONCAT_IMPL(a, b)

#define TEST(name)                                                                                                     \
	static void name();                                                                                                \
	static const ::sw::test::TestRegistrar SW_TEST_CONCAT(name, _registrar){#name, &name, {__FILE__, __LINE__}};       \
	static void name()

// Запятая внутри {} делит аргументы макроса, такие выражения берутся в скобки:
// CHECK_EQ(range.shifted(-1, -1), (DistanceRange{1, 3})).
#define CHECK(expression)                                                                                              \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(expression))                                                                                             \
		{                                                                                                              \
			::sw::test::reportFailure({__FILE__, __LINE__}, "CHECK(" #expression ") failed");                          \
		}                                                                                                              \
	}                                                                                                                  \
	while (false)

#define CHECK_EQ(actual, expected)                                                                                     \
	do                                                                                                                 \
	{                                                                                                                  \
		const auto& sw_test_actual = (actual);                                                                         \
		const auto& sw_test_expected = (expected);                                                                     \
		if (!(sw_test_actual == sw_test_expected))                                                                     \
		{                                                                                                              \
			::sw::test::reportFailure(                                                                                 \
				{__FILE__, __LINE__},                                                                                  \
				"CHECK_EQ(" #actual ", " #expected ") failed: " + ::sw::test::detail::describe(sw_test_actual)         \
					+ " != " + ::sw::test::detail::describe(sw_test_expected));                                        \
		}                                                                                                              \
	}                                                                                                                  \
	while (false)

#define CHECK_THROWS(expression, exception_type)                                                                       \
	do                                                                                                                 \
	{                                                                                                                  \
		const char* sw_test_problem = "expected exception was not thrown";                                             \
		try                                                                                                            \
		{                                                                                                              \
			static_cast<void>(expression);                                                                             \
		}                                                                                                              \
		catch (const exception_type&)                                                                                  \
		{                                                                                                              \
			sw_test_problem = nullptr;                                                                                 \
		}                                                                                                              \
		catch (...)                                                                                                    \
		{                                                                                                              \
			sw_test_problem = "exception of another type was thrown";                                                  \
		}                                                                                                              \
		if (sw_test_problem != nullptr)                                                                                \
		{                                                                                                              \
			::sw::test::reportFailure(                                                                                 \
				{__FILE__, __LINE__},                                                                                  \
				std::string("CHECK_THROWS(" #expression ", " #exception_type ") failed: ") + sw_test_problem);         \
		}                                                                                                              \
	}                                                                                                                  \
	while (false)
