cmake_minimum_required(VERSION 3.25)
project(Failboy LANGUAGES C)

option(FAILBOY_TESTS "Fetch Blargg's test ROMs and register them with CTest" ON)
option(FAILBOY_SDL "Fetch SDL3 and build the windowed front end" ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

# The emulator core and the headless runner; front ends live in subdirectories of src.
file(GLOB FAILBOY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_executable(failboy ${FAILBOY_SOURCES})
target_include_directories(failboy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
	target_compile_options(failboy PRIVATE -Wall -Wextra -pedantic)
endif()

# On Windows the release builds are windowed programs, so starting one doesn't open a console as well.
set_target_properties(failboy PROPERTIES WIN32_EXECUTABLE "$<CONFIG:Release,MinSizeRel>")

# Link-time optimisation for the optimised builds (lets the compiler inline across source files).
include(CheckIPOSupported)
check_ipo_supported(RESULT FAILBOY_IPO OUTPUT ipo_output LANGUAGES C)
if(FAILBOY_IPO)
	set_target_properties(failboy PROPERTIES
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

if(FAILBOY_TESTS)
	enable_testing()

	# Blargg's test ROMs (data only, pinned to the repository's single commit).
	FetchContent_Declare(gb_test_roms
		GIT_REPOSITORY https://github.com/retrio/gb-test-roms.git
		GIT_TAG c240dd7d700e5c0b00a7bbba52b53e4ee67b5f15)
	FetchContent_MakeAvailable(gb_test_roms)

	add_test(NAME instr_timing COMMAND failboy --headless "${gb_test_roms_SOURCE_DIR}/instr_timing/instr_timing.gb")
	set(rom_tests instr_timing)

	set(roms "${gb_test_roms_SOURCE_DIR}/cpu_instrs")
	add_test(NAME cpu_instrs COMMAND failboy --headless "${roms}/cpu_instrs.gb")
	list(APPEND rom_tests cpu_instrs)

	file(GLOB individual "${roms}/individual/*.gb")
	foreach(rom IN LISTS individual)
		# "03-op sp,hl.gb" -> "03-op_sp_hl"
		get_filename_component(name "${rom}" NAME_WE)
		string(REGEX REPLACE "[^A-Za-z0-9-]+" "_" name "${name}")
		string(REGEX REPLACE "_$" "" name "${name}")
		add_test(NAME ${name} COMMAND failboy --headless "${rom}")
		list(APPEND rom_tests ${name})
	endforeach()

	# Matt Currie's dmg-acid2 PPU test: the rendered frame has to match the reference image exactly.
	# EXPECTED is the SHA256 of img/reference-dmg.png converted to the PGM that --dump writes.
	FetchContent_Declare(dmg_acid2
		URL https://github.com/mattcurrie/dmg-acid2/releases/download/v1.0/dmg-acid2.gb
		URL_HASH SHA256=464e14b7d42e7feea0b7ede42be7071dc88913f75b9ffa444299424b63d1dff1
		DOWNLOAD_NO_EXTRACT TRUE)
	FetchContent_MakeAvailable(dmg_acid2)
	add_test(NAME dmg-acid2 COMMAND "${CMAKE_COMMAND}"
		"-DFAILBOY=$<TARGET_FILE:failboy>"
		"-DROM=${dmg_acid2_SOURCE_DIR}/dmg-acid2.gb"
		-DFRAMES=10
		"-DOUT=${CMAKE_CURRENT_BINARY_DIR}/dmg-acid2.pgm"
		-DEXPECTED=b2fd578e3564e82e82b4e796d9e4bf735c36fa26a52d55c78132eb225aad72e3
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/frame_test.cmake")
	list(APPEND rom_tests dmg-acid2)

	set_tests_properties(${rom_tests} PROPERTIES TIMEOUT 60)
endif()
