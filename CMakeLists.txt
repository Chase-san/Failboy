cmake_minimum_required(VERSION 3.31)
project(failboy LANGUAGES C)

# Set C standard to C17
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# DWARF for LLDB debugging
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_options(-gdwarf)
endif()

set(SRC_FILES
  src/cart.c
  src/cpu_instr_cb.c
  src/cpu_instr.c
  src/cpu.c
  src/io.c
  src/mem.c
  src/ppu.c
)

add_executable(${PROJECT_NAME} src/main.c ${SRC_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall
  -Wextra
  -Werror
  -pedantic
  -Wno-unused-function
  -Wno-unused-parameter
)