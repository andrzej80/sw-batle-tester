#pragma once

#include "Core/Simulation/SimulationTestHelpers.hpp"

#include <App/Application.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

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

	// EventLog пишет в std::cout: на время теста подменяем буфер
	class CoutCapture
	{
	public:
		CoutCapture() :
				_previous(std::cout.rdbuf(_buffer.rdbuf()))
		{}

		~CoutCapture()
		{
			std::cout.rdbuf(_previous);
		}

		CoutCapture(const CoutCapture&) = delete;
		CoutCapture& operator=(const CoutCapture&) = delete;

		std::string text() const
		{
			return _buffer.str();
		}

	private:
		std::ostringstream _buffer;
		std::streambuf* _previous;
	};

	struct AppRun
	{
		int code;
		std::string out;  // stdout — события
		std::string err;  // stderr — сообщения приложения
	};

	// Запуск приложения на тексте сценария, как из командной строки. По умолчанию —
	// настоящие адаптеры и детерминированная случайность (FirstRandom).
	inline AppRun runApplication(
		const std::string& name, const std::string& scenario, app::ApplicationOptions options = {})
	{
		options.random = []
		{
			return std::make_unique<FirstRandom>();
		};
		const TempScenario file(name, scenario);
		const std::string path = file.path();
		const char* argv[] = {"sw_battle_test", path.c_str()};

		std::ostringstream errors;
		const CoutCapture capture;
		const int code = app::Application(std::move(options)).run(2, argv, errors);

		// Путь к временному файлу в сообщениях заменяется на "scenario.txt": тексты сравнимы
		std::string err = errors.str();
		for (size_t at = err.find(path); at != std::string::npos; at = err.find(path, at))
		{
			err.replace(at, path.size(), "scenario.txt");
		}
		return {code, capture.text(), err};
	}
}
