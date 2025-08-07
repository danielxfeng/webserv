cmake_minimum_required(VERSION 3.14)
project(WebServerTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Google Test subdirectory
add_subdirectory(libs/gtest)

# Enable testing
enable_testing()

# Add test executable(s)
add_executable(testtest test/test/test.cpp)

target_include_directories(testtest PRIVATE ${CMAKE_SOURCE_DIR}/src/include)

# Link Google Test libraries to your tests
target_link_libraries(testtest PRIVATE gtest_main)

include(GoogleTest)
gtest_discover_tests(testtest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

#cmake -S . -B build
#cmake --build build
#cd ./build
#ctest cpp09ex00 --verbose