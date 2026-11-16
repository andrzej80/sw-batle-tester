#pragma once

#include "Registration/Registrar.hpp"

#include <Core/Common/Types.hpp>
#include <Core/Random/IRandom.hpp>
#include <Core/Simulation/Simulation.hpp>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace sw::app
{
	namespace exit_code
	{
		inline constexpr int Success = 0;
		inline constexpr int Environment = 1;
		inline constexpr int InvalidScenario = 2;
		inline constexpr int InternalError = 3;
	}

	struct ApplicationOptions
	{
		std::vector<BindingFn> bindings = bindingList();
		std::vector<OutputFn> outputs = outputList();
		std::function<std::unique_ptr<core::IRandom>()> random = []
		{
			return std::make_unique<core::StdRandom>(core::StdRandom::seedFromDevice());
		};
		core::Tick tickLimit = core::Simulation::DefaultTickLimit;
	};

	class Application
	{
	public:
		explicit Application(ApplicationOptions options = {});

		int run(int argc, const char* const* argv, std::ostream& errors) const;

	private:
		int processFile(const std::string& path, std::ostream& errors) const;

		ApplicationOptions _options;
	};
}
