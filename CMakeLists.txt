cmake_minimum_required(VERSION 3.10)
project(CoreProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main library
add_library(shealth_lib
    src/main/cpp/SHealth.cpp
)
target_include_directories(shealth_lib PUBLIC src/main/cpp)

# Main executable
add_executable(SHealthBMI src/main/cpp/SHealthBMI.cpp)
target_link_libraries(SHealthBMI PRIVATE shealth_lib)

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test executable
add_executable(SHealthBMITest src/test/cpp/SHealthBMITest.cpp)
target_link_libraries(SHealthBMITest PRIVATE shealth_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(SHealthBMITest)
