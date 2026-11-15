# Проверка границ слоёв (ARCHITECTURE.md, п. 2): какой слой какие слои может включать.
#
#   cmake -DSW_SOURCE_DIR=<каталог src> [-DSW_STAMP=<файл>] -P CheckLayerBoundaries.cmake
#
# Слой файла — первый каталог относительно src/. Проверяются все #include в .cpp и .hpp
# слоёв; находятся ВСЕ нарушения сразу, при нарушениях скрипт завершается с ошибкой.
# Файлы прямо в src/ (main.cpp) — точка сборки приложения, их включения не ограничены.

cmake_minimum_required(VERSION 3.27)

if (NOT DEFINED SW_SOURCE_DIR)
	message(FATAL_ERROR "SW_SOURCE_DIR is not set")
endif()
get_filename_component(source_dir "${SW_SOURCE_DIR}" ABSOLUTE)

set(layers Core Features IO App)
set(allowed_Core Core)
set(allowed_Features Core Features)
set(allowed_IO IO)
set(allowed_App Core Features IO App)

# Слой по пути: первый каталог относительно src/; пусто — путь не принадлежит слою
# (стандартная библиотека, файл вне src/).
function(sw_layer_of path out)
	file(RELATIVE_PATH relative "${source_dir}" "${path}")
	string(REGEX MATCH "^[^/]+" first "${relative}")
	if (NOT relative MATCHES "^\\.\\." AND first IN_LIST layers AND relative MATCHES "/")
		set(${out} "${first}" PARENT_SCOPE)
	else()
		set(${out} "" PARENT_SCOPE)
	endif()
endfunction()

# Куда указывает #include: "…" — сначала относительно файла, затем через include-каталог src/;
# <…> — через include-каталог src/.
function(sw_resolve_include file kind target out)
	if (kind STREQUAL "\"")
		get_filename_component(directory "${file}" DIRECTORY)
		get_filename_component(near "${directory}/${target}" ABSOLUTE)
		if (EXISTS "${near}" OR NOT EXISTS "${source_dir}/${target}")
			set(${out} "${near}" PARENT_SCOPE)
			return()
		endif()
	endif()
	get_filename_component(through_src "${source_dir}/${target}" ABSOLUTE)
	set(${out} "${through_src}" PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE files "${source_dir}/*.cpp" "${source_dir}/*.hpp")
list(SORT files)

set(violations "")
set(count 0)
foreach (file IN LISTS files)
	sw_layer_of("${file}" file_layer)
	if (file_layer STREQUAL "")
		continue()
	endif()

	file(STRINGS "${file}" include_lines REGEX "^[ \t]*#[ \t]*include" ENCODING UTF-8)
	foreach (line IN LISTS include_lines)
		if (NOT line MATCHES "#[ \t]*include[ \t]*([<\"])([^>\"]+)[>\"]")
			continue()
		endif()
		set(kind "${CMAKE_MATCH_1}")
		set(target "${CMAKE_MATCH_2}")

		sw_resolve_include("${file}" "${kind}" "${target}" resolved)
		sw_layer_of("${resolved}" target_layer)
		if (target_layer STREQUAL "" OR target_layer IN_LIST allowed_${file_layer})
			continue()
		endif()

		file(RELATIVE_PATH shown "${source_dir}" "${file}")
		if (kind STREQUAL "<")
			set(shown_include "<${target}>")
		else()
			set(shown_include "\"${target}\"")
		endif()
		list(JOIN allowed_${file_layer} ", " allowed_text)
		string(APPEND violations
			"  ${shown}: #include ${shown_include} is layer ${target_layer}; ${file_layer} may include only: ${allowed_text}\n")
		math(EXPR count "${count} + 1")
	endforeach()
endforeach()

if (count GREATER 0)
	message(FATAL_ERROR "found ${count} layer boundary violation(s) (ARCHITECTURE.md, section 2):\n${violations}")
endif()

if (DEFINED SW_STAMP)
	file(TOUCH "${SW_STAMP}")
endif()
message(STATUS "layer boundaries: ok (${source_dir})")
