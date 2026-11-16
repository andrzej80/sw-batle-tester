#include "TestFramework/TestFramework.hpp"

#include <Core/Common/Result.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using sw::core::Diagnostic;
using sw::core::Diagnostics;
using sw::core::Result;
using sw::core::UnitId;

namespace
{
	Diagnostic duplicateId()
	{
		return {"unit.duplicate_id", "unit id 1 already exists", UnitId{1}};
	}

	Result<int> parsePositive(int value)
	{
		if (value <= 0)
		{
			return Diagnostic{"value.not_positive", "value must be positive"};
		}
		return value;
	}
}

TEST(Result_holds_a_value)
{
	const Result<int> result = parsePositive(5);
	CHECK(result.ok());
	CHECK_EQ(result.value(), 5);
	CHECK(result.errors().empty());
}

TEST(Result_holds_errors)
{
	const Result<int> result = parsePositive(-1);
	CHECK(!result.ok());
	CHECK_EQ(result.errors().size(), size_t{1});
	CHECK_EQ(result.errors()[0].code, std::string("value.not_positive"));
	CHECK_THROWS(static_cast<void>(result.value()), std::logic_error);
}

TEST(Result_keeps_all_errors)
{
	Diagnostics errors;
	errors.add("unit.invalid_stat", "hp must be positive, got 0");
	errors.add(duplicateId());
	const Result<int> result = errors;
	CHECK_EQ(result.errors().size(), size_t{2});
	CHECK_EQ(result.errors()[1], duplicateId());
}

TEST(Result_requires_at_least_one_error)
{
	CHECK_THROWS(Result<int>(Diagnostics{}), std::logic_error);
	CHECK_THROWS(Result<void>(Diagnostics{}), std::logic_error);
}

TEST(Result_supports_move_only_values)
{
	Result<std::unique_ptr<int>> result = std::make_unique<int>(42);
	CHECK(result.ok());
	const std::unique_ptr<int> value = std::move(result).value();
	CHECK_EQ(*value, 42);
}

TEST(Result_void_is_success_or_errors)
{
	const Result<void> success{};
	CHECK(success.ok());
	CHECK(success.errors().empty());

	const Result<void> failure = duplicateId();
	CHECK(!failure.ok());
	CHECK_EQ(failure.errors()[0].relatedUnit.value(), UnitId{1});
}

TEST(Result_errors_of_all_steps_can_be_collected_without_ok_checks)
{
	const Result<void> steps[] = {Result<void>{}, Diagnostic{"a", "first"}, Result<void>{}, Diagnostic{"b", "second"}};
	Diagnostics all;
	for (const Result<void>& step : steps)
	{
		all.append(step.errors());
	}
	CHECK_EQ(all.size(), size_t{2});
	CHECK_EQ(all[0].code, std::string("a"));
	CHECK_EQ(all[1].code, std::string("b"));
}

// Регрессия: ASan нашёл stack-use-after-scope в цикле по errors() временного результата
TEST(Result_errors_of_a_temporary_can_be_iterated_safely)
{
	std::vector<std::string> codes;
	for (const Diagnostic& error : parsePositive(-1).errors())
	{
		codes.push_back(error.code);
	}
	for (const Diagnostic& error : Result<void>(duplicateId()).errors())
	{
		codes.push_back(error.code);
	}
	CHECK(codes == (std::vector<std::string>{"value.not_positive", "unit.duplicate_id"}));
	CHECK(parsePositive(1).errors().empty());
}

TEST(Diagnostics_append_keeps_order)
{
	Diagnostics first{{"a", "first"}};
	const Diagnostics second{{"b", "second"}, {"c", "third"}};
	first.append(second);

	CHECK_EQ(first.size(), size_t{3});
	CHECK_EQ(first[0].code, std::string("a"));
	CHECK_EQ(first[2].code, std::string("c"));
}
