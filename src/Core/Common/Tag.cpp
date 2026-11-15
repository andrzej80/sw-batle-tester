#include "Tag.hpp"

#include <algorithm>
#include <deque>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace sw::core
{
	namespace
	{
		class TagRegistry
		{
		public:
			static TagRegistry& instance()
			{
				static TagRegistry registry;
				return registry;
			}

			size_t intern(std::string_view name)
			{
				const std::string key(name);
				if (const auto found = _ids.find(key); found != _ids.end())
				{
					return found->second;
				}
				const size_t id = _names.size();
				_names.push_back(key);
				_ids.emplace(key, id);
				return id;
			}

			std::string_view name(size_t id) const
			{
				return _names.at(id);
			}

		private:
			// deque: ссылки на имена не меняются при добавлении, name() отдаёт string_view на них
			std::deque<std::string> _names;
			std::unordered_map<std::string, size_t> _ids;
		};
	}

	namespace
	{
		std::string_view requireName(std::string_view name)
		{
			if (name.empty())
			{
				throw std::logic_error("Tag: name must not be empty");
			}
			return name;
		}
	}

	Tag::Tag(std::string_view name) :
			_id(TagRegistry::instance().intern(requireName(name)))
	{}

	std::string_view Tag::name() const
	{
		return TagRegistry::instance().name(_id);
	}

	std::ostream& operator<<(std::ostream& stream, Tag tag)
	{
		return stream << tag.name();
	}

	TagSet::TagSet(std::initializer_list<Tag> tags)
	{
		for (const Tag tag : tags)
		{
			add(tag);
		}
	}

	void TagSet::add(Tag tag)
	{
		const auto position = std::lower_bound(_tags.begin(), _tags.end(), tag);
		if (position == _tags.end() || *position != tag)
		{
			_tags.insert(position, tag);
		}
	}

	bool TagSet::has(Tag tag) const
	{
		return std::binary_search(_tags.begin(), _tags.end(), tag);
	}

	bool TagSet::hasAny(const TagSet& other) const
	{
		return std::any_of(other._tags.begin(), other._tags.end(), [this](Tag tag) { return has(tag); });
	}

	std::ostream& operator<<(std::ostream& stream, const TagSet& tags)
	{
		stream << '{';
		const char* separator = "";
		for (const Tag tag : tags)
		{
			stream << separator << tag;
			separator = ", ";
		}
		return stream << '}';
	}
}
