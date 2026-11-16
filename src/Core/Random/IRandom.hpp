#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

namespace sw::core
{
	class IRandom
	{
	public:
		IRandom() = default;
		virtual ~IRandom() = default;

		IRandom(const IRandom&) = delete;
		IRandom& operator=(const IRandom&) = delete;
		IRandom(IRandom&&) = delete;
		IRandom& operator=(IRandom&&) = delete;

		virtual size_t index(size_t count) = 0;
	};

	class StdRandom final : public IRandom
	{
	public:
		explicit StdRandom(uint64_t seed);

		static uint64_t seedFromDevice();

		size_t index(size_t count) override;

	private:
		std::mt19937_64 _engine;
	};
}
