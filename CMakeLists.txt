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
add_executable(testtinyjson test/tinyjson/testTinyJson.cpp srcs/TinyJson.cpp)

if(TARGET testtest)
  target_include_directories(testtest PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
endif()

if(TARGET testtinyjson)
  target_include_directories(testtinyjson PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
endif()

# Link Google Test libraries to your tests
target_link_libraries(testtest PRIVATE gtest_main)
target_link_libraries(testtinyjson PRIVATE gtest_main)

include(GoogleTest)
gtest_discover_tests(testtest)
gtest_discover_tests(testtinyjson)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

#cmake -S . -B build
#cmake --build build
#cd ./build
#ctest cpp09ex00 --verbose