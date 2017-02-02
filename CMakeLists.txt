cmake_minimum_required(VERSION 3.6)

project (LC3Simulator)

set (LC3Simulator_VERSION_MAJOR 0)
set (LC3Simulator_VERSION_MINOR 1)

set (SOURCE_FILES
	source/Error.c
	source/LC3.c
	source/Logging.c
	source/Machine.c
	source/Main.c
	source/Memory.c
	source/OptParse.c
	source/Parser.c
)

include_directories (${PROJECT_SOURCE_DIR}/includes)

add_executable (LC3Simulator ${SOURCE_FILES})

find_package (Curses REQUIRED)
if (CURSES_FOUND)
    include_directories (${CURSES_INCLUDE_DIR})
    target_link_libraries (LC3Simulator ${CURSES_LIBRARIES})
else ()
	message (SEND_ERROR "This program requires the curses library.")
endif (CURSES_FOUND)

option (DEBUG "Build the debug version" OFF)

if (DEBUG)
	target_compile_options (LC3Simulator PRIVATE
		-O0 -pg -std=c11 -Wall -Werror -Wpedantic -Wextra -Wcast-align
		-Wfloat-equal -Wmissing-braces -Wmissing-declarations
		-Wmissing-field-initializers -Wmissing-prototypes
		-Woverlength-strings -Wparentheses -Wreturn-type -Wshadow
		-Wsign-compare -Wsign-conversion -Wswitch -Wswitch-enum
		-Wuninitialized -Wunknown-pragmas -Wunreachable-code
		-Wunused-function -Wunused-label -Wunused-parameter
		-Wunused-value -Wunused-variable
	)

	target_link_libraries (LC3Simulator -pg)

	set (CMAKE_VERBOSE_MAKEFILE ON)
else ()
	target_compile_options (LC3Simulator PRIVATE -O2 -std=c11)
endif (DEBUG)

add_definitions (-DOS_PATH=${PROJECT_SOURCE_DIR})

