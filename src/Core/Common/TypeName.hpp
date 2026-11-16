#pragma once

#include <string>
#include <typeinfo>

namespace sw::core
{
	std::string typeName(const std::type_info& type);

	template <class T>
	std::string typeName()
	{
		return typeName(typeid(T));
	}

	template <class T>
	std::string typeName(const T& object)
	{
		return typeName(typeid(object));
	}
}
