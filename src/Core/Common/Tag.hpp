#pragma once

#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <string_view>
#include <vector>

namespace sw::core
{
	class Tag
	{
	public:
		explicit Tag(std::string_view name);

		std::string_view name() const;

		friend bool operator==(Tag, Tag) = default;
		friend auto operator<=>(Tag, Tag) = default;

	private:
		size_t _id;
	};

	std::ostream& operator<<(std::ostream& stream, Tag tag);

	class TagSet
	{
	public:
		TagSet() = default;
		TagSet(std::initializer_list<Tag> tags);

		void add(Tag tag);
		bool has(Tag tag) const;
		bool hasAny(const TagSet& other) const;

		bool empty() const
		{
			return _tags.empty();
		}

		size_t size() const
		{
			return _tags.size();
		}

		auto begin() const
		{
			return _tags.begin();
		}

		auto end() const
		{
			return _tags.end();
		}

		friend bool operator==(const TagSet&, const TagSet&) = default;

	private:
		std::vector<Tag> _tags;
	};

	std::ostream& operator<<(std::ostream& stream, const TagSet& tags);
}
