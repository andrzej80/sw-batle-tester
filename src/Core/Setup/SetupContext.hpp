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

	struct SpawnRequest
	{
		UnitId id;
		Position position;
		bool occupiesCell;
		Result<Unit> unit;
	};

	class SetupContext
	{
	public:
		explicit SetupContext(const EventBus& events);
		~SetupContext();

		SetupContext(const SetupContext&) = delete;
		SetupContext& operator=(const SetupContext&) = delete;
		SetupContext(SetupContext&&) = delete;
		SetupContext& operator=(SetupContext&&) = delete;

		Result<void> createMap(int64_t width, int64_t height);

		Result<void> spawn(SpawnRequest request);

		Result<void> orderMarch(UnitId id, Position target);

		Result<void> finish();

		void noteFailedMapCommand()
		{
			_mapCommandSeen = true;
		}

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
