cmake_minimum_required(VERSION 3.20)
project(pricer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # needed to link the core into the Python module
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)     # lets your IDE (CLion, VS Code) understand the project

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(PRICER_BUILD_TESTS  "Build the GoogleTest suite"     ON)
option(PRICER_BUILD_PYTHON "Build the pybind11 module"      ON)

# ---------------------------------------------------------------------------
# Core library: pure C++, no dependency. Everything else is built on top of it.
# ---------------------------------------------------------------------------
add_library(pricer_core
  core/src/black_scholes.cpp
  core/src/implied_vol.cpp
  core/src/monte_carlo.cpp
)
target_include_directories(pricer_core PUBLIC core/include)
target_compile_options(pricer_core PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion>
)

# ---------------------------------------------------------------------------
# Tests (GoogleTest is downloaded at configure time)
# ---------------------------------------------------------------------------
if(PRICER_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_executable(pricer_tests
    tests/test_black_scholes.cpp
    tests/test_implied_vol.cpp
    tests/test_monte_carlo.cpp
  )
  target_link_libraries(pricer_tests PRIVATE pricer_core GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(pricer_tests)
endif()

# ---------------------------------------------------------------------------
# Python module: `import pricer` from Python, written to ./python/
# ---------------------------------------------------------------------------
if(PRICER_BUILD_PYTHON)
  find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
  find_package(pybind11 CONFIG REQUIRED)
  pybind11_add_module(pricer bindings/module.cpp)
  target_link_libraries(pricer PRIVATE pricer_core)
  set_target_properties(pricer PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/python)
endif()
