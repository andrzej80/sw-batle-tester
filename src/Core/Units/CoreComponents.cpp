#include "CoreComponents.hpp"

#include <stdexcept>
#include <string>

namespace sw::core
{
	namespace
	{
		int64_t requirePositive(int64_t value, const char* what)
		{
			if (value <= 0)
			{
				throw std::logic_error(std::string(what) + " must be positive, got " + std::to_string(value));
			}
			return value;
		}
	}

	Health::Health(int64_t initial) :
			current(requirePositive(initial, "Health")),
			max(initial)
	{}

	Movement::Movement(int64_t speed) :
			speed(requirePositive(speed, "Movement speed"))
	{}

	Status::Status(std::optional<Tick> expiresAt) :
			_expiresAt(expiresAt)
	{}
}
