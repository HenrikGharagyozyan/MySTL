cmake_minimum_required(VERSION 3.14)
project(MySTL VERSION 1.0.0 LANGUAGES CXX)

# Require a modern standard (C++20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable strict warnings to ensure code reliability
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Header-only library target
add_library(mystl INTERFACE)

target_include_directories(mystl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(mystl INTERFACE cxx_std_20)

# Integrate GoogleTest for unit testing
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()
add_subdirectory(tests)