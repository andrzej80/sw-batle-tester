#include "TestFramework/TestFramework.hpp"

#include <IO/Commands/CreateMap.hpp>
#include <IO/Commands/March.hpp>
#include <IO/Commands/SpawnHunter.hpp>
#include <IO/Commands/SpawnSwordsman.hpp>
#include <IO/System/CommandParser.hpp>
#include <IO/System/ParseNumber.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using sw::io::CommandParser;
using sw::io::ParseDiagnostic;
using sw::io::parseUInt32;

namespace
{
	// Команда со строковым полем: формат допускает и такие поля
	struct Say
	{
		constexpr static const char* Name = "SAY";
		std::string word{};
		uint32_t times{};

		template <typename Visitor>
		void visit(Visitor& visitor)
		{
			visitor.visit("word", word);
			visitor.visit("times", times);
		}
	};

	struct Parsed
	{
		std::vector<std::string> commands;
		std::vector<ParseDiagnostic> errors;
	};

	Parsed parse(const std::string& scenario)
	{
		Parsed result;
		CommandParser parser;
		parser
			.add<sw::io::CreateMap>(
				[&](sw::io::CreateMap c, size_t line)
				{
					result.commands.push_back(
						std::to_string(line) + ": CREATE_MAP " + std::to_string(c.width) + " "
						+ std::to_string(c.height));
				})
			.add<sw::io::SpawnHunter>(
				[&](sw::io::SpawnHunter c, size_t line)
				{
					result.commands.push_back(
						std::to_string(line) + ": SPAWN_HUNTER " + std::to_string(c.unitId)
						+ " range=" + std::to_string(c.range));
				})
			.add<sw::io::March>(
				[&](sw::io::March c, size_t line)
				{
					result.commands.push_back(
						std::to_string(line) + ": MARCH " + std::to_string(c.unitId) + " " + std::to_string(c.targetX)
						+ " " + std::to_string(c.targetY));
				})
			.add<Say>(
				[&](Say c, size_t line) {
					result.commands.push_back(
						std::to_string(line) + ": SAY " + c.word + " x" + std::to_string(c.times));
				});
		std::istringstream stream(scenario);
		result.errors = parser.parse(stream);
		return result;
	}

	std::string describe(const ParseDiagnostic& error)
	{
		return std::to_string(error.line) + " " + error.command + " [" + error.code + "] " + error.message;
	}

	std::vector<std::string> describe(const std::vector<ParseDiagnostic>& errors)
	{
		std::vector<std::string> list;
		for (const ParseDiagnostic& error : errors)
		{
			list.push_back(describe(error));
		}
		return list;
	}

	std::string uintResult(const std::string& token)
	{
		std::string error;
		const auto value = parseUInt32(token, error);
		return value ? std::to_string(*value) : "error: " + error;
	}
}

TEST(CommandParser_dispatches_commands_with_physical_line_numbers)
{
	const Parsed parsed = parse(
		"CREATE_MAP 10 10\n"
		"\n"
		"// комментарий\n"
		"SPAWN_HUNTER 2 9 0 10 5 1 4\n"
		"   \t \n"
		"MARCH 2 0 0\n");

	CHECK(parsed.errors.empty());
	CHECK(
		parsed.commands
		== (std::vector<std::string>{"1: CREATE_MAP 10 10", "4: SPAWN_HUNTER 2 range=4", "6: MARCH 2 0 0"}));
}

TEST(CommandParser_accepts_extra_spaces_tabs_and_windows_line_endings)
{
	const Parsed parsed = parse("CREATE_MAP   10\t10  \r\nMARCH 1 2 3\r\n");
	CHECK(parsed.errors.empty());
	CHECK(parsed.commands == (std::vector<std::string>{"1: CREATE_MAP 10 10", "2: MARCH 1 2 3"}));
}

TEST(CommandParser_reads_string_fields)
{
	const Parsed parsed = parse("SAY hello 3\n");
	CHECK(parsed.commands == (std::vector<std::string>{"1: SAY hello x3"}));
}

// Приёмка SPECIFICATION.md, п. 8
TEST(CommandParser_reports_unknown_command)
{
	const Parsed parsed = parse("SPAWN_DRAGON 1 0 0\n");
	CHECK(
		describe(parsed.errors)
		== (std::vector<std::string>{"1  [parse.unknown_command] unknown command 'SPAWN_DRAGON'"}));
}

TEST(CommandParser_reports_first_missing_argument_by_field_name)
{
	const Parsed parsed = parse("SPAWN_HUNTER 1 0 0 10 5 1\n");
	CHECK(
		describe(parsed.errors)
		== (std::vector<std::string>{"1 SPAWN_HUNTER [parse.missing_argument] missing argument 'range'"}));
	CHECK(parsed.commands.empty());
}

TEST(CommandParser_reports_invalid_number_by_field_name)
{
	const Parsed parsed = parse("CREATE_MAP 10 x\n");
	CHECK(
		describe(parsed.errors)
		== (std::vector<std::string>{
			"1 CREATE_MAP [parse.invalid_number] height: expected non-negative integer, got \"x\""}));
}

TEST(CommandParser_reports_negative_number_instead_of_wrapping_it)
{
	const Parsed parsed = parse("SPAWN_HUNTER 1 0 0 -5 5 1 4\n");
	CHECK(
		describe(parsed.errors)
		== (std::vector<std::string>{
			"1 SPAWN_HUNTER [parse.invalid_number] hp: expected non-negative integer, got \"-5\""}));
}

TEST(CommandParser_reports_extra_argument)
{
	const Parsed parsed = parse("MARCH 1 2 3 4\n");
	CHECK(
		describe(parsed.errors)
		== (std::vector<std::string>{"1 MARCH [parse.extra_argument] unexpected extra argument '4'"}));
	CHECK(parsed.commands.empty());
}

TEST(CommandParser_collects_errors_of_all_lines_and_skips_only_broken_ones)
{
	const Parsed parsed = parse(
		"CREATE_MAP 10 10\n"
		"FLY 1\n"
		"MARCH 1 2\n"
		"MARCH 1 2 3\n"
		"CREATE_MAP 4294967296 1\n");

	CHECK(parsed.commands == (std::vector<std::string>{"1: CREATE_MAP 10 10", "4: MARCH 1 2 3"}));
	CHECK(
		describe(parsed.errors)
		== (std::vector<std::string>{
			"2  [parse.unknown_command] unknown command 'FLY'",
			"3 MARCH [parse.missing_argument] missing argument 'targetY'",
			"5 CREATE_MAP [parse.invalid_number] width: value exceeds 4294967295, got 4294967296"}));
}

TEST(CommandParser_rejects_duplicate_and_empty_handlers)
{
	CommandParser parser;
	parser.add<sw::io::March>([](sw::io::March, size_t) {});
	CHECK_THROWS(parser.add<sw::io::March>([](sw::io::March, size_t) {}), std::logic_error);
	CHECK_THROWS(parser.add<Say>(std::function<void(Say, size_t)>{}), std::logic_error);
}

// Таблица приёмки п. 8.1
TEST(ParseUInt32_accepts_only_plain_decimal_digits)
{
	CHECK_EQ(uintResult("-5"), std::string("error: expected non-negative integer, got \"-5\""));
	CHECK_EQ(uintResult("+5"), std::string("error: expected non-negative integer, got \"+5\""));
	CHECK_EQ(uintResult("5x"), std::string("error: expected non-negative integer, got \"5x\""));
	CHECK_EQ(uintResult("1.5"), std::string("error: expected non-negative integer, got \"1.5\""));
	CHECK_EQ(uintResult("0x10"), std::string("error: expected non-negative integer, got \"0x10\""));
	CHECK_EQ(uintResult(""), std::string("error: expected non-negative integer, got \"\""));
	CHECK_EQ(uintResult("4294967295"), std::string("4294967295"));
	CHECK_EQ(uintResult("4294967296"), std::string("error: value exceeds 4294967295, got 4294967296"));
	CHECK_EQ(uintResult("007"), std::string("7"));
	CHECK_EQ(uintResult("0"), std::string("0"));
}
