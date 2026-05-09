cmake_minimum_required(VERSION 3.16)
project(DSA_Guide C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build flags
add_compile_options(-Wall -Wextra -g -O1)

include_directories(include)

# ── Collect all implementation sources ──────────────────────────────────────
file(GLOB_RECURSE DSA_SOURCES
    src/linear/*.c
    src/trees/*.c
    src/hash/*.c
    src/graph/*.c
    src/probabilistic/*.c
    src/spatial/*.c
    src/specialized/*.c
)

# ── Shared library (all DS implementations) ──────────────────────────────────
add_library(dsa STATIC ${DSA_SOURCES})
target_include_directories(dsa PUBLIC include)

# ── Demo runner ──────────────────────────────────────────────────────────────
add_executable(dsa_demo src/main.c)
target_link_libraries(dsa_demo dsa m)

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()
file(GLOB TEST_SOURCES tests/*.c)
foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} dsa m)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

message(STATUS "DSA Guide — ${CMAKE_BUILD_TYPE} build")
message(STATUS "Sources found: ${DSA_SOURCES}")
