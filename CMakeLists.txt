cmake_minimum_required(VERSION 3.25)
project(Failboy LANGUAGES C)

option(FAILBOY_SDL "Fetch SDL3 and build the windowed front end" ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

# The emulator core is a library: everything in src itself, with src/failboy.h all that its hosts use of it. The
# hosts live in subdirectories of src: the command line (and headless runner) in src/cli, the window in src/sdl.
file(GLOB FAILBOY_CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_library(failboy_core STATIC ${FAILBOY_CORE_SOURCES})
target_include_directories(failboy_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(failboy src/cli/failboy.c src/cli/files.c)
target_link_libraries(failboy PRIVATE failboy_core)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
	target_compile_options(failboy_core PRIVATE -Wall -Wextra -pedantic)
	target_compile_options(failboy PRIVATE -Wall -Wextra -pedantic)
endif()

# On Windows the release builds are windowed programs, so starting one doesn't open a console as well.
set_target_properties(failboy PROPERTIES WIN32_EXECUTABLE "$<CONFIG:Release,MinSizeRel>")

# Link-time optimisation for the optimised builds (lets the compiler inline across source files, and from the core
# into its hosts).
include(CheckIPOSupported)
check_ipo_supported(RESULT FAILBOY_IPO OUTPUT ipo_output LANGUAGES C)
if(FAILBOY_IPO)
	set_target_properties(failboy_core failboy PROPERTIES
		INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
		INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
		INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
else()
	message(STATUS "LTO isn't available: ${ipo_output}")
endif()

if(FAILBOY_SDL)
	# An installed SDL3 is used if find_package() finds one; otherwise the pinned release is built.
	FetchContent_Declare(SDL3
		URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
		URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
		FIND_PACKAGE_ARGS 3.4 CONFIG)
	set(SDL_SHARED OFF CACHE BOOL "" FORCE)
	set(SDL_STATIC ON CACHE BOOL "" FORCE)
	set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(SDL3)
	target_sources(failboy PRIVATE src/sdl/frontend.c src/sdl/rslcd.c)
	target_compile_definitions(failboy PRIVATE FAILBOY_SDL)
	target_link_libraries(failboy PRIVATE SDL3::SDL3)
endif()


# The spot tests in src/tests: each a program of its own, making the cartridges it needs.
option(FAILBOY_TESTS "Build the spot tests and register them with CTest" ON)
if(FAILBOY_TESTS)
	enable_testing()
	add_library(failboy_test STATIC src/tests/test.c)
	target_link_libraries(failboy_test PUBLIC failboy_core)
	set(test_targets failboy_test)
	foreach(name api cart timer revision speed)
		add_executable(test_${name} src/tests/${name}.c)
		target_link_libraries(test_${name} PRIVATE failboy_test)
		add_test(NAME ${name} COMMAND test_${name})
		list(APPEND test_targets test_${name})
	endforeach()
	if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
		foreach(target IN LISTS test_targets)
			target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
		endforeach()
	endif()

	# Where the shootout's test ROMs are (tests/roms, which isn't in the repository), some of them get the api test's
	# check that a run comes out the same from a new Game Boy, a reset one, and one made after others.
	set(same_again_roms "")
	foreach(rom acid/dmg-acid2.gb acid/cgb-acid2.gbc blargg/instr_timing.gb blargg/dmg_sound/04-sweep.gb
			mooneye/acceptance/oam_dma/basic.gb samesuite/dma/hdma_mode0.gb)
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/roms/${rom}")
			list(APPEND same_again_roms "${CMAKE_CURRENT_SOURCE_DIR}/tests/roms/${rom}")
		endif()
	endforeach()
	if(same_again_roms)
		add_test(NAME api_roms COMMAND test_api ${same_again_roms})
	endif()
endif()
