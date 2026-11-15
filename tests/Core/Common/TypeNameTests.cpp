#include "TestFramework/TestFramework.hpp"

#include <Core/Common/DistanceRange.hpp>
#include <Core/Common/TypeName.hpp>
#include <memory>
#include <string>

namespace
{
	struct Base
	{
		virtual ~Base() = default;
	};

	struct Derived final : Base
	{};
}

TEST(TypeName_is_readable)
{
	CHECK_EQ(sw::core::typeName<sw::core::DistanceRange>(), std::string("sw::core::DistanceRange"));
	CHECK_EQ(sw::core::typeName<int>(), std::string("int"));
}

TEST(TypeName_of_object_is_its_dynamic_type)
{
	const std::unique_ptr<Base> object = std::make_unique<Derived>();
	CHECK_EQ(sw::core::typeName(*object), std::string("(anonymous namespace)::Derived"));
}
