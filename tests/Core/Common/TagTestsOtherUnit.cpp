#include "TagTestsOtherUnit.hpp"

namespace sw::test
{
	sw::core::Tag fireTagFromOtherUnit()
	{
		static const sw::core::Tag fire{"fire"};
		return fire;
	}
}
