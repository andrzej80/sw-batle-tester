#include "App/AppTestHelpers.hpp"
#include "TestFramework/TestFramework.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::string readFile(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		std::ostringstream content;
		content << file.rdbuf();
		return content.str();
	}

	std::vector<std::filesystem::path> scenarioDirectories()
	{
		std::vector<std::filesystem::path> directories;
		for (const auto& entry : std::filesystem::directory_iterator(SW_SCENARIOS_DIR))
		{
			if (entry.is_directory() && std::filesystem::exists(entry.path() / "scenario.txt"))
			{
				directories.push_back(entry.path());
			}
		}
		std::sort(directories.begin(), directories.end());
		return directories;
	}

	void compare(
		const std::string& scenario, const std::string& stream, const std::string& actual, const std::string& expected)
	{
		if (actual != expected)
		{
			sw::test::reportFailure(
				{__FILE__, __LINE__},
				scenario + ": " + stream + " differs\n--- expected ---\n" + expected + "--- actual ---\n" + actual);
		}
	}
}

TEST(GoldenScenarios_match_expected_output)
{
	const std::vector<std::filesystem::path> directories = scenarioDirectories();
	CHECK(directories.size() >= 9);

	for (const std::filesystem::path& directory : directories)
	{
		const std::string name = directory.filename().string();
		const sw::test::AppRun run = sw::test::runApplication("golden_" + name, readFile(directory / "scenario.txt"));

		compare(name, "stdout", run.out, readFile(directory / "expected.out"));
		compare(name, "stderr", run.err, readFile(directory / "expected.err"));
		compare(name, "exit code", std::to_string(run.code) + "\n", readFile(directory / "expected.code"));
	}
}
