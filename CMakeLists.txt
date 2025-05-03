cmake_minimum_required(VERSION 3.25)
project(Swift LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FILE(GLOB_RECURSE SWIFT_SOURCES src/*.cpp)
add_library(${PROJECT_NAME} ${SWIFT_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)

if (SWIFT_EXAMPLES)
    add_subdirectory(examples)
endif ()
add_subdirectory(external)