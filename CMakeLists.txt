CMAKE_MINIMUM_REQUIRED ( VERSION 3.6 )

PROJECT ( LC3Simulator )
SET ( PROJECT LC3Simulator )

SET ( CMAKE_C_STANDARD 11 )

SET ( LC3Simulator_VERSION_MAJOR 0 )
SET ( LC3Simulator_VERSION_MINOR 1 )

SET ( SOURCE_FILES
      source/Error.c
      source/LC3.c
      source/Logging.c
      source/Machine.c
      source/Main.c
      source/Memory.c
      source/OptParse.c
      source/Parser.c
      )

ADD_EXECUTABLE ( ${PROJECT} ${SOURCE_FILES} )

INCLUDE_DIRECTORIES ( ${PROJECT_SOURCE_DIR}/includes )

FIND_PACKAGE ( Curses REQUIRED )
IF ( CURSES_FOUND )
    TARGET_LINK_LIBRARIES ( ${PROJECT} ${CURSES_LIBRARIES} )
ELSE ()
    MESSAGE ( SEND_ERROR "This program requires the curses library." )
ENDIF ()

IF ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -pg -Wall -Werror -Wpedantic -Wextra" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align -Wfloat-equal -Wmissing-braces" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-field-initializers" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes -Woverlength-strings" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wparentheses -Wreturn-type -Wshadow -Wsign-compare" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-conversion -Wswitch -Wswitch-enum" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wuninitialized -Wunknown-pragmas -Wunreachable-code" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused-function -Wunused-label -Wunused-parameter" )
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused-value -Wunused-variable" )

    SET ( CMAKE_VERBOSE_MAKEFILE ON )
ELSE ()
    SET ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2" )
ENDIF ()

ADD_DEFINITIONS ( -DOS_PATH=${PROJECT_SOURCE_DIR} )


