#pragma once

#include <Core/Common/Result.hpp>
#include <Core/Common/Types.hpp>
#include <Core/Units/Unit.hpp>
#include <cstdint>
#include <memory>
#include <unordered_set>

namespace sw::core
{
	class EventBus;
	class MovementService;
	class World;

	// Заявка на спавн: что фабрика юнита знает всегда, даже если статы неверны.
	// По id, позиции и занятости клетки проверки идут и тогда, когда юнит не собрался:
	// у спавна с плохими статами и занятой клеткой видны обе ошибки.
	struct SpawnRequest
	{
		UnitId id;
		Position position;
		bool occupiesCell;
		Result<Unit> unit;	// собранный юнит или ошибки фабрики
	};

	// Выполнение команд сценария (тик 1) с проверкой входных данных (SPECIFICATION.md,
	// п. 7). Каждый метод возвращает все найденные ошибки своего вызова; ошибки ввода —
	// Result, а не исключения. Используется в обоих проходах: в проходе проверки события
	// уходят в шину без подписчиков.
	//
	// Ошибки-последствия подавляются — первопричина уже в списке:
	// - MARCH на юнита, чей спавн отклонён, ошибок не даёт;
	// - после CREATE_MAP с ошибкой команды, которым нужна карта, не дают map.not_created.
	class SetupContext
	{
	public:
		explicit SetupContext(const EventBus& events);
		~SetupContext();

		SetupContext(const SetupContext&) = delete;
		SetupContext& operator=(const SetupContext&) = delete;
		SetupContext(SetupContext&&) = delete;
		SetupContext& operator=(SetupContext&&) = delete;

		// map.already_created — повторная команда; map.invalid_size — W или H ≤ 0.
		// Успех — MapCreated.
		Result<void> createMap(int64_t width, int64_t height);

		// Ошибки фабрики как есть; map.not_created — карты ещё нет (один раз за сценарий);
		// unit.duplicate_id, unit.out_of_bounds, unit.cell_occupied. Успех — UnitSpawned.
		Result<void> spawn(SpawnRequest request);

		// march.unit_not_found (в том числе MARCH до SPAWN), march.unit_cannot_move,
		// march.out_of_bounds, march.same_position (цель совпадает с концом очереди, а при
		// пустой очереди — с позицией). Первый марш юнита — MarchStarted, следующие — в очередь.
		Result<void> orderMarch(UnitId id, Position target);

		// Конец сценария: CREATE_MAP так и не было — map.not_created (если ещё не сообщалось)
		Result<void> finish();

		// Построенный мир; nullptr — карта не создана
		World* world()
		{
			return _world.get();
		}

	private:
		bool requireMap(Diagnostics& errors);

		const EventBus& _events;
		std::unique_ptr<World> _world;
		std::unique_ptr<MovementService> _movement;
		bool _mapCommandSeen = false;
		bool _missingMapReported = false;
		std::unordered_set<UnitId> _rejected;
	};
}
