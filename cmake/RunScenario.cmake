# Сценарный тест настоящего исполняемого файла (tests/scenarios/README.md):
#
#   cmake -DSW_BINARY=<sw_battle_test> -DSW_SCENARIO_DIR=<каталог сценария> -P RunScenario.cmake
#
# Запускает sw_battle_test scenario.txt в каталоге сценария (поэтому в сообщениях путь —
# scenario.txt) и сравнивает stdout, stderr и код выхода с expected.out / .err / .code.

cmake_minimum_required(VERSION 3.27)

foreach (required SW_BINARY SW_SCENARIO_DIR)
	if (NOT DEFINED ${required})
		message(FATAL_ERROR "${required} is not set")
	endif()
endforeach()

execute_process(
	COMMAND "${SW_BINARY}" scenario.txt
	WORKING_DIRECTORY "${SW_SCENARIO_DIR}"
	OUTPUT_VARIABLE actual_out
	ERROR_VARIABLE actual_err
	RESULT_VARIABLE actual_code)

file(READ "${SW_SCENARIO_DIR}/expected.out" expected_out)
file(READ "${SW_SCENARIO_DIR}/expected.err" expected_err)
file(READ "${SW_SCENARIO_DIR}/expected.code" expected_code)
string(STRIP "${expected_code}" expected_code)

set(failures "")
if (NOT actual_code STREQUAL expected_code)
	string(APPEND failures "exit code: expected ${expected_code}, got ${actual_code}\n")
endif()
if (NOT actual_out STREQUAL expected_out)
	string(APPEND failures "stdout differs\n--- expected ---\n${expected_out}--- actual ---\n${actual_out}")
endif()
if (NOT actual_err STREQUAL expected_err)
	string(APPEND failures "stderr differs\n--- expected ---\n${expected_err}--- actual ---\n${actual_err}")
endif()

if (failures)
	message(FATAL_ERROR "scenario ${SW_SCENARIO_DIR}:\n${failures}")
endif()
