#pragma once

#include <string>
#include <typeinfo>

namespace sw::core
{
	// Читаемое имя типа для сообщений о багах: "sw::core::Health", а не "N2sw4core6HealthE".
	// Компонентам и действиям не нужно объявлять свои имена.
	std::string typeName(const std::type_info& type);

	template <class T>
	std::string typeName()
	{
		return typeName(typeid(T));
	}

	// Имя динамического типа объекта (полиморфного — реального, а не базового)
	template <class T>
	std::string typeName(const T& object)
	{
		return typeName(typeid(object));
	}
}
