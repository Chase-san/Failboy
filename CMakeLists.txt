cmake_minimum_required(VERSION 3.25)
project(Failboy LANGUAGES C)

option(FAILBOY_TESTS "Fetch Blargg's test ROMs and register them with CTest" ON)
option(FAILBOY_SDL "Fetch SDL3 and build the windowed front end" OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

file(GLOB FAILBOY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_executable(failboy ${FAILBOY_SOURCES})

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
	target_compile_options(failboy PRIVATE -Wall -Wextra -pedantic)
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
	target_compile_definitions(failboy PRIVATE FAILBOY_SDL)
	target_link_libraries(failboy PRIVATE SDL3::SDL3)
endif()

if(FAILBOY_TESTS)
	enable_testing()

	# Blargg's test ROMs (data only, pinned to the repository's single commit).
	FetchContent_Declare(gb_test_roms
		GIT_REPOSITORY https://github.com/retrio/gb-test-roms.git
		GIT_TAG c240dd7d700e5c0b00a7bbba52b53e4ee67b5f15)
	FetchContent_MakeAvailable(gb_test_roms)

	add_test(NAME instr_timing COMMAND failboy "${gb_test_roms_SOURCE_DIR}/instr_timing/instr_timing.gb")
	set(rom_tests instr_timing)

	set(roms "${gb_test_roms_SOURCE_DIR}/cpu_instrs")
	add_test(NAME cpu_instrs COMMAND failboy "${roms}/cpu_instrs.gb")
	list(APPEND rom_tests cpu_instrs)

	file(GLOB individual "${roms}/individual/*.gb")
	foreach(rom IN LISTS individual)
		# "03-op sp,hl.gb" -> "03-op_sp_hl"
		get_filename_component(name "${rom}" NAME_WE)
		string(REGEX REPLACE "[^A-Za-z0-9-]+" "_" name "${name}")
		string(REGEX REPLACE "_$" "" name "${name}")
		add_test(NAME ${name} COMMAND failboy "${rom}")
		list(APPEND rom_tests ${name})
	endforeach()

	set_tests_properties(${rom_tests} PROPERTIES TIMEOUT 60)
endif()
