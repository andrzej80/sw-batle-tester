#include "TagTestsOtherUnit.hpp"
#include "TestFramework/TestFramework.hpp"

#include <Core/Common/Tag.hpp>
#include <sstream>
#include <stdexcept>

using sw::core::Tag;
using sw::core::TagSet;

namespace
{
	const Tag Fire{"fire"};
	const Tag Melee{"melee"};
	const Tag Ranged{"ranged"};
}

TEST(Tag_with_the_same_name_is_the_same_tag_across_translation_units)
{
	CHECK(Fire == sw::test::fireTagFromOtherUnit());
	CHECK(Tag("fire") == Fire);
	CHECK(Fire != Melee);
}

TEST(Tag_keeps_its_name)
{
	CHECK_EQ(Fire.name(), std::string_view("fire"));
	std::ostringstream stream;
	stream << Melee;
	CHECK_EQ(stream.str(), std::string("melee"));
}

TEST(Tag_requires_a_name)
{
	CHECK_THROWS(Tag(""), std::logic_error);
}

TEST(TagSet_has_and_has_any)
{
	const TagSet attack{Melee, Fire};
	CHECK(attack.has(Melee));
	CHECK(attack.has(Fire));
	CHECK(!attack.has(Ranged));
	CHECK(attack.hasAny(TagSet{Ranged, Fire}));
	CHECK(!attack.hasAny(TagSet{Ranged}));
	CHECK(!attack.hasAny(TagSet{}));
}

TEST(TagSet_ignores_duplicates_and_order)
{
	TagSet tags{Fire, Melee, Fire};
	CHECK_EQ(tags.size(), size_t{2});
	CHECK(tags == (TagSet{Melee, Fire}));

	tags.add(Melee);
	CHECK_EQ(tags.size(), size_t{2});
	CHECK(TagSet{}.empty());
}
