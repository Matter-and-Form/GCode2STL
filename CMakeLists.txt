cmake_minimum_required(VERSION 3.22)
project(GCode2STL VERSION 1.0.0 LANGUAGES C CXX)

# Explicitly set the output directories for all build configurations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include(CTest)
enable_testing()

add_executable(GCode2STL main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
