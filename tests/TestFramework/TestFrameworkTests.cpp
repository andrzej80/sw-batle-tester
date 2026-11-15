// Тесты самого фреймворка: проверки считают провалы, не прерывают тест и
// сообщают понятный текст.

#include "TestFramework.hpp"

#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
	bool contains(const std::string& text, const std::string& fragment)
	{
		return text.find(fragment) != std::string::npos;
	}

	void throwLogicError()
	{
		throw std::logic_error("boom");
	}

	void throwRuntimeError()
	{
		throw std::runtime_error("boom");
	}

	void doNothing() {}
}

TEST(TestFramework_passing_checks_report_no_failures)
{
	std::ostringstream output;
	const size_t failures = sw::test::runIsolated(
		[]
		{
			CHECK(1 + 1 == 2);
			CHECK_EQ(std::string("abc"), std::string("abc"));
			CHECK_THROWS(throwLogicError(), std::logic_error);
		},
		output);

	CHECK_EQ(failures, size_t{0});
	CHECK(output.str().empty());
}

TEST(TestFramework_failed_check_does_not_stop_the_test)
{
	std::ostringstream output;
	const size_t failures = sw::test::runIsolated(
		[]
		{
			CHECK(false);
			CHECK(2 < 1);
		},
		output);

	CHECK_EQ(failures, size_t{2});
	CHECK(contains(output.str(), "CHECK(false) failed"));
	CHECK(contains(output.str(), "CHECK(2 < 1) failed"));
}

TEST(TestFramework_check_eq_reports_both_values)
{
	std::ostringstream output;
	const size_t failures = sw::test::runIsolated([] { CHECK_EQ(3, 4); }, output);

	CHECK_EQ(failures, size_t{1});
	CHECK(contains(output.str(), "3 != 4"));
}

TEST(TestFramework_check_throws_detects_missing_and_wrong_exception)
{
	std::ostringstream output;
	const size_t failures = sw::test::runIsolated(
		[]
		{
			CHECK_THROWS(doNothing(), std::logic_error);
			CHECK_THROWS(throwRuntimeError(), std::logic_error);
		},
		output);

	CHECK_EQ(failures, size_t{2});
	CHECK(contains(output.str(), "expected exception was not thrown"));
	CHECK(contains(output.str(), "exception of another type was thrown"));
}

TEST(TestFramework_unexpected_exception_is_a_failure)
{
	std::ostringstream output;
	const size_t failures = sw::test::runIsolated([] { throwRuntimeError(); }, output);

	CHECK_EQ(failures, size_t{1});
	CHECK(contains(output.str(), "unexpected exception: boom"));
}
