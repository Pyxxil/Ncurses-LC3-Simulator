cmake_minimum_required(VERSION 3.7)

project (LC3Simulator)

set (LC3Simulator_VERSION_MAJOR 0)
set (LC3Simulator_VERSION_MINOR 1)

include_directories ($(PROJECT_SOURCE_DIR)/includes)

add_executable(LC3Simulator
	source/Error.c
	source/LC3.c
	source/Logging.c
	source/Machine.c
	source/Main.c
	source/Memory.c
	source/OptParse.c
	source/Parser.c
)

find_package (Curses REQUIRED)
if (CURSES_FOUND)
	include_directories (${CURSES_INCLUDE_DIR})
	target_link_libraries (LC3Simulator ${CURSES_LIBRARIES})
endif (CURSES_FOUND)

option (CMAKEVERBOSE
	"Make CMake verbose" OFF)

if (CMAKEVERBOSE)
	set (CMAKE_VERBOSE_MAKEFILE ON)
endif (CMAKEVERBOSE)

option (DEBUG
	"Build the debug version" OFF)

if (DEBUG)
	target_compile_options (LC3Simulator PRIVATE -O0 -g -std=c11)
else ()
	target_compile_options (LC3Simulator PRIVATE -O2 -std=c11)
endif (DEBUG)

add_definitions (-DOS_PATH=${PROJECT_SOURCE_DIR})

