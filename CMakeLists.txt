cmake_minimum_required(VERSION 3.27)
project(sw_battle_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SW_SANITIZE "Build with UndefinedBehaviorSanitizer and implicit-conversion checks (clang only)" OFF)
option(SW_BUILD_TESTS "Build tests" ON)

# Общие флаги всех целей проекта (SPECIFICATION.md, п. 11).
# Предупреждения как ошибки — стандартной опцией CMake, как в CI:
#   cmake -DCMAKE_COMPILE_WARNING_AS_ERROR=ON ...
add_library(sw_options INTERFACE)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
	target_compile_options(sw_options INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

if (SW_SANITIZE)
	if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(FATAL_ERROR "SW_SANITIZE requires clang: implicit-conversion sanitizer is clang-only")
	endif()
	# -fno-sanitize-recover: нарушение роняет программу, а не только печатается,
	# иначе тест с нарушением всё равно считался бы пройденным.
	# unsigned-integer-overflow не включается: он срабатывает на std::mt19937 в libstdc++.
	set(SW_SANITIZER_FLAGS -fsanitize=undefined,implicit-conversion -fno-sanitize-recover=all)
	target_compile_options(sw_options INTERFACE ${SW_SANITIZER_FLAGS} -fno-omit-frame-pointer)
	target_link_options(sw_options INTERFACE ${SW_SANITIZER_FLAGS})
endif()

# Проверка границ слоёв (ARCHITECTURE.md, п. 2): Core не включает другие слои, Features —
# только Core, IO — только себя. Выполняется в каждой сборке любой цели проекта и
# перезапускается при изменении любого исходника; при нарушениях сборка падает со списком всех.
set(SW_LAYER_CHECK_SCRIPT "${PROJECT_SOURCE_DIR}/cmake/CheckLayerBoundaries.cmake")
set(SW_LAYER_CHECK_STAMP "${PROJECT_BINARY_DIR}/layer_boundaries.stamp")
file(GLOB_RECURSE sw_all_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/src/*.hpp")
add_custom_command(
	OUTPUT "${SW_LAYER_CHECK_STAMP}"
	COMMAND "${CMAKE_COMMAND}" "-DSW_SOURCE_DIR=${PROJECT_SOURCE_DIR}/src" "-DSW_STAMP=${SW_LAYER_CHECK_STAMP}"
		-P "${SW_LAYER_CHECK_SCRIPT}"
	DEPENDS ${sw_all_sources} "${SW_LAYER_CHECK_SCRIPT}"
	COMMENT "Checking layer boundaries"
	VERBATIM)
add_custom_target(sw_layer_boundaries DEPENDS "${SW_LAYER_CHECK_STAMP}")

# Слой — каталог в src/. Файлы собираются по маске, а не перечислением: общих
# списков, которые правят все разработчики, в проекте нет (ARCHITECTURE.md, п. 2).
# Пока в слое нет ни одного .cpp, он подключается как INTERFACE-библиотека.
function(sw_add_layer target kind directory)
	file(GLOB_RECURSE layer_sources CONFIGURE_DEPENDS "${directory}/*.cpp")
	file(GLOB_RECURSE layer_headers CONFIGURE_DEPENDS "${directory}/*.hpp")
	if (layer_sources)
		add_library(${target} ${kind} ${layer_sources} ${layer_headers})
		target_include_directories(${target} PUBLIC "${PROJECT_SOURCE_DIR}/src")
		target_link_libraries(${target} PUBLIC sw_options ${ARGN})
	else()
		add_library(${target} INTERFACE)
		target_include_directories(${target} INTERFACE "${PROJECT_SOURCE_DIR}/src")
		target_link_libraries(${target} INTERFACE sw_options ${ARGN})
	endif()
	add_dependencies(${target} sw_layer_boundaries)
endfunction()

sw_add_layer(sw_core STATIC "${PROJECT_SOURCE_DIR}/src/Core")
sw_add_layer(sw_features STATIC "${PROJECT_SOURCE_DIR}/src/Features" sw_core)
sw_add_layer(sw_io STATIC "${PROJECT_SOURCE_DIR}/src/IO")

# ВАЖНО: App — только OBJECT-библиотека (или исходники прямо в исполняемом файле),
# никогда STATIC. Адаптеры команд и событий регистрируются статическими объектами
# (SW_REGISTER_BINDING / SW_REGISTER_OUTPUT); из статической библиотеки линкер
# выбросит файлы, на которые никто не ссылается, и команды молча пропадут
# (ARCHITECTURE.md, п. 5.4).
sw_add_layer(sw_app OBJECT "${PROJECT_SOURCE_DIR}/src/App" sw_core sw_features sw_io)

add_executable(sw_battle_test "${PROJECT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(sw_battle_test PRIVATE sw_app)

if (SW_BUILD_TESTS)
	enable_testing()
	file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS
		"${PROJECT_SOURCE_DIR}/tests/*.cpp" "${PROJECT_SOURCE_DIR}/tests/*.hpp")
	# fixtures/ — входные данные тестов, а не код тестов
	list(FILTER test_sources EXCLUDE REGEX "/fixtures/")
	add_executable(sw_tests ${test_sources})
	target_include_directories(sw_tests PRIVATE "${PROJECT_SOURCE_DIR}/tests")
	target_link_libraries(sw_tests PRIVATE sw_app)
	add_test(NAME unit_tests COMMAND sw_tests)

	# Сама проверка границ слоёв: на наборе файлов с заведомыми нарушениями она обязана
	# найти их все, а на настоящем src/ — не найти ни одного.
	set(sw_layer_fixtures "${PROJECT_SOURCE_DIR}/tests/LayerBoundaries/fixtures")
	add_test(NAME layer_boundaries_reports_all_violations
		COMMAND "${CMAKE_COMMAND}" "-DSW_SOURCE_DIR=${sw_layer_fixtures}/src" -P "${SW_LAYER_CHECK_SCRIPT}")
	set_tests_properties(layer_boundaries_reports_all_violations PROPERTIES
		PASS_REGULAR_EXPRESSION
		"found 3 layer boundary violation.*Core/BadCore.hpp: #include <IO/Commands/Fake.hpp> is layer IO.*Features/BadFeature.hpp: #include \"../App/Fake.hpp\" is layer App.*IO/BadIo.hpp: #include <Core/Helper.hpp> is layer Core")
	add_test(NAME layer_boundaries_project_is_clean
		COMMAND "${CMAKE_COMMAND}" "-DSW_SOURCE_DIR=${PROJECT_SOURCE_DIR}/src" -P "${SW_LAYER_CHECK_SCRIPT}")
endif()
