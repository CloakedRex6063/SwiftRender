cmake_minimum_required(VERSION 3.27)
project(Swift)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PUBLIC inc)
target_include_directories(${PROJECT_NAME} PRIVATE src)
file(GLOB_RECURSE SWIFT_SOURCES src/*.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${SWIFT_SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC SWIFT_DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PUBLIC SWIFT_RELEASE)
endif ()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SWIFT_WINDOWS)
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SWIFT_LINUX)
endif ()

add_subdirectory(extern)

set(SWIFT_EXAMPLES ON)

if(SWIFT_EXAMPLES)
    add_subdirectory(examples)
endif ()