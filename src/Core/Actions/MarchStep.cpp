#include "MarchStep.hpp"

#include <Core/Map/Map.hpp>
#include <Core/Simulation/TurnContext.hpp>
#include <Core/Units/ComponentRequirements.hpp>
#include <Core/Units/CoreComponents.hpp>
#include <Core/Units/Unit.hpp>
#include <Core/World/SpatialQuery.hpp>
#include <algorithm>
#include <vector>

namespace sw::core
{
	namespace
	{
		int64_t sign(int64_t value)
		{
			return (value > 0) - (value < 0);
		}

		std::vector<Position> candidateCells(const Unit& self, Position target, const SpatialQuery& query)
		{
			const Position from = self.position();
			const int64_t distance = chebyshev(from, target);
			std::vector<Position> candidates;
			for (const Position cell : query.map().neighbourCells(from))
			{
				if (chebyshev(cell, target) < distance && query.isFreeFor(self, cell))
				{
					candidates.push_back(cell);
				}
			}
			return candidates;
		}

		Position chooseCell(const std::vector<Position>& candidates, Position from, Position target, IRandom& random)
		{
			const Position direct{from.x + sign(target.x - from.x), from.y + sign(target.y - from.y)};
			if (std::find(candidates.begin(), candidates.end(), direct) != candidates.end())
			{
				return direct;
			}
			return candidates[random.index(candidates.size())];
		}
	}

	void MarchStep::requirements(ComponentRequirements& requirements) const
	{
		requirements.require<Movement>();
	}

	bool MarchStep::tryExecute(const Unit& self, TurnContext& context)
	{
		const Movement& movement = self.get<Movement>();
		if (movement.marches.empty())
		{
			return false;
		}
		const Position target = movement.marches.front();

		int64_t budget = movement.speed;
		bool moved = false;
		while (budget > 0 && self.position() != target && isAlive(self))
		{
			const std::vector<Position> candidates = candidateCells(self, target, context.query());
			if (candidates.empty())
			{
				break;
			}
			const Position cell = chooseCell(candidates, self.position(), target, context.random());
			const int64_t cost = context.query().map().moveCost(cell, self);
			if (moved && cost > budget)
			{
				break;
			}
			if (!context.movement().stepTo(self, cell, context))
			{
				break;
			}
			budget -= cost;
			moved = true;
		}

		if (self.position() == target)
		{
			context.movement().finishMarch(self);
			return true;
		}
		return moved;
	}
}
