#pragma once

#include <cstdint>
#include <string>

namespace sw::io
{
	struct MineExploded
	{
		constexpr static const char* Name = "MINE_EXPLODED";

		uint32_t unitId{};

		template <typename Visitor>
		void visit(Visitor& visitor)
		{
			visitor.visit("unitId", unitId);
		}
	};
}
