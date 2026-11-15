#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace sw::test
{
	// Временный файл сценария; удаляется в деструкторе
	class TempScenario
	{
	public:
		TempScenario(const std::string& name, const std::string& content) :
				_path(std::filesystem::temp_directory_path() / ("sw_battle_test_" + name + ".txt"))
		{
			std::ofstream file(_path);
			file << content;
		}

		~TempScenario()
		{
			std::error_code ignored;
			std::filesystem::remove(_path, ignored);
		}

		TempScenario(const TempScenario&) = delete;
		TempScenario& operator=(const TempScenario&) = delete;

		std::string path() const
		{
			return _path.string();
		}

	private:
		std::filesystem::path _path;
	};
}
