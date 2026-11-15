#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

namespace sw::core
{
	// Источник случайности передаётся в Core снаружи (ARCHITECTURE.md, п. 4.12): в
	// приложении — StdRandom, в тестах — фиксированный seed или заглушка.
	class IRandom
	{
	public:
		IRandom() = default;
		virtual ~IRandom() = default;

		IRandom(const IRandom&) = delete;
		IRandom& operator=(const IRandom&) = delete;
		IRandom(IRandom&&) = delete;
		IRandom& operator=(IRandom&&) = delete;

		// Случайный индекс в [0, count); count == 0 — std::logic_error
		virtual size_t index(size_t count) = 0;
	};

	class StdRandom final : public IRandom
	{
	public:
		explicit StdRandom(uint64_t seed);

		// seed от std::random_device: каждый запуск приложения — своя последовательность
		static uint64_t seedFromDevice();

		size_t index(size_t count) override;

	private:
		std::mt19937_64 _engine;
	};
}
