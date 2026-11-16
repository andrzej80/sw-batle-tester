#include "TestFramework.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <utility>

namespace sw::test
{
	namespace
	{
		struct RunContext
		{
			size_t failures{};
			std::ostream* output{};
		};

		RunContext*& currentContext()
		{
			static RunContext* context = nullptr;
			return context;
		}
	}

	std::vector<TestCase>& registry()
	{
		static std::vector<TestCase> tests;
		return tests;
	}

	TestRegistrar::TestRegistrar(const char* name, TestFunction function, SourceLocation location)
	{
		registry().push_back({name, function, location});
	}

	void reportFailure(SourceLocation location, const std::string& message)
	{
		RunContext* context = currentContext();
		if (context == nullptr)
		{
			throw std::logic_error("test check used outside of a running test: " + message);
		}
		++context->failures;
		*context->output << "    " << location.file << ":" << location.line << ": " << message << "\n";
	}

	size_t runIsolated(const std::function<void()>& body, std::ostream& output)
	{
		RunContext context{0, &output};
		RunContext* previous = std::exchange(currentContext(), &context);
		try
		{
			body();
		}
		catch (const std::exception& exception)
		{
			++context.failures;
			output << "    unexpected exception: " << exception.what() << "\n";
		}
		catch (...)
		{
			++context.failures;
			output << "    unexpected exception of unknown type\n";
		}
		currentContext() = previous;
		return context.failures;
	}

	int runAll(std::string_view filter, std::ostream& output)
	{
		// Порядок регистрации между файлами не определён — сортируем, чтобы отчёт был стабильным
		std::vector<TestCase> tests = registry();
		std::sort(
			tests.begin(),
			tests.end(),
			[](const TestCase& a, const TestCase& b)
			{
				const int byFile = std::strcmp(a.location.file, b.location.file);
				return byFile != 0 ? byFile < 0 : a.location.line < b.location.line;
			});

		size_t run = 0;
		size_t failed = 0;
		for (const TestCase& test : tests)
		{
			if (!filter.empty() && test.name.find(filter) == std::string::npos)
			{
				continue;
			}
			++run;
			std::ostringstream details;
			if (runIsolated(test.function, details) == 0)
			{
				output << "[ OK   ] " << test.name << "\n";
			}
			else
			{
				++failed;
				output << "[ FAIL ] " << test.name << " (" << test.location.file << ":" << test.location.line << ")\n"
					   << details.str();
			}
		}

		output << "\n" << run - failed << " passed, " << failed << " failed, " << run << " total\n";
		if (run == 0)
		{
			output << "no tests matched filter '" << filter << "'\n";
			return 1;
		}
		return failed == 0 ? 0 : 1;
	}
}
