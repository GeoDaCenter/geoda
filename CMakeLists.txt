cmake_minimum_required(VERSION 3.27...3.27)

project(geoda
    DESCRIPTION "GeoDa: An introduction to spatial data analysis"
    LANGUAGES CXX
    VERSION 1.24.0
)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_VERBOSE_MAKEFILE ON)

# sources
include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(src)

# dependencies
include(FetchContent)
find_package(Boost 1.75.0)
if(!Boost_FOUND)
    FetchContent_Declare(boost
        URL https://boostorg.jfrog.io/artifactory/main/release/1.75.0/source/boost_1_75_0.tar.bz2
    )
    FetchContent_MakeAvailable(boost)
    FetchContent_GetProperties(boost SOURCE_DIR Boost_INCLUDE_DIRS)
endif()

set(wxWidgets_CONFIG_EXECUTABLE ${CMAKE_SOURCE_DIR}/BuildTools/macosx/libraries/bin/wx-config)
find_package(wxWidgets)
message(wxWidgets_INCLUDE_DIRS="${wxWidgets_INCLUDE_DIRS}")
message(wxWidgets_LIBRARIES="${wxWidgets_LIBRARIES}")
message(wxWidgets_LIBRARY_DIRS="${wxWidgets_LIBRARY_DIRS}")
message(wxWidgets_DEFINITIONS="${wxWidgets_DEFINITIONS}")

# options
option(UNITTEST "Unit test" ON)
if (UNITTEST)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()