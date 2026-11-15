#include "IRandom.hpp"

#include <stdexcept>

namespace sw::core
{
	StdRandom::StdRandom(uint64_t seed) :
			_engine(seed)
	{}

	uint64_t StdRandom::seedFromDevice()
	{
		std::random_device device;
		const uint64_t high = device();
		const uint64_t low = device();
		return (high << 32U) ^ low;
	}

	size_t StdRandom::index(size_t count)
	{
		if (count == 0)
		{
			throw std::logic_error("IRandom: index() of an empty range");
		}
		std::uniform_int_distribution<size_t> distribution(0, count - 1);
		return distribution(_engine);
	}
}
