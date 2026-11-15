#pragma once

#include "MovementService.hpp"
#include "TurnContext.hpp"

#include <Core/Common/Types.hpp>
#include <Core/Interaction/Interaction.hpp>
#include <Core/Interaction/Targeting.hpp>
#include <cstdint>
#include <optional>

namespace sw::core
{
	class EventBus;
	class IRandom;
	class World;

	enum class StopReason : std::uint8_t
	{
		OneOrNoUnitsLeft,  // на карте ≤ 1 юнита
		NothingHappened,   // тик пустой и отложенных эффектов нет
		TickLimit,		   // страховка от бесконечной симуляции
	};

	// Ход из трёх фаз (ARCHITECTURE.md, п. 4.4; SPECIFICATION.md, п. 6.4):
	// 1. Scheduled — отложенные эффекты тика в порядке постановки;
	// 2. Main — юниты в порядке создания (снимок на начало фазы); мёртвые и помеченные на
	//    удаление пропускаются; выполняется первое действие Behaviour, вернувшее true;
	// 3. Cleanup — удаление мёртвых и помеченных, снятие истёкших статусов, проверка конца.
	//
	// Контракт IAction проверяется всегда: действие вернуло false, но мир изменился —
	// std::logic_error с именем действия и юнита.
	class Simulation
	{
	public:
		static constexpr Tick DefaultTickLimit = 30'000;

		// Тик 1 (команды сценария) уже прошёл: мир построен
		Simulation(World& world, const EventBus& events, IRandom& random, Tick tickLimit = DefaultTickLimit);

		Simulation(const Simulation&) = delete;
		Simulation& operator=(const Simulation&) = delete;
		Simulation(Simulation&&) = delete;
		Simulation& operator=(Simulation&&) = delete;

		// До конца. Если после установки на карте ≤ 1 юнита, ходы не начинаются.
		StopReason run();

		// Один тик; причина остановки, если после него симуляция заканчивается
		std::optional<StopReason> runTick();

		// Контекст хода — для отложенных эффектов и тестов действий
		TurnContext& context()
		{
			return _context;
		}

	private:
		void runScheduledPhase();
		void runMainPhase();
		void runCleanupPhase();

		World& _world;
		Tick _tickLimit;
		Targeting _targeting;
		Interaction _interaction;
		MovementService _movement;
		TurnContext _context;
	};
}
