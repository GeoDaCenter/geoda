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
if(NOT Boost_FOUND)
    FetchContent_Declare(boost
        URL https://boostorg.jfrog.io/artifactory/main/release/1.75.0/source/boost_1_75_0.tar.bz2
    )
    FetchContent_MakeAvailable(boost)
    FetchContent_GetProperties(boost SOURCE_DIR Boost_INCLUDE_DIRS)
endif()

find_package(wxWidgets)
if(NOT wxWidgets_FOUND)
    FetchContent_Declare(
		wxWidgets
		URL https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.2.1/wxWidgets-3.2.2.1.tar.bz2
	)
	set(FETCHCONTENT_QUIET NO)
    if(APPLE)
		set(wxUSE_LIBSDL OFF CACHE BOOL "Use SDL for wxSound implementation.")
	endif()
    set(wxBUILD_INSTALL ON)
    set(wxBUILD_SHARED OFF CACHE BOOL "Build wx libraries as shared libs")
    set(wxBUILD_MONOLITHIC ON CACHE BOOL "Build a single library")
    set(wxBUILD_PRECOMP ON CACHE BOOL "Use precompiled headers")
    set(wxUSE_GLCANVAS ON)
    set(wxUSE_POSTSCRIPT ON)
    set(wxUSE_TEXTFILE ON)
    set(wxUSE_LIBLZMA OFF)
    set(wxUSE_WEBVIEW ON)
    set(wxUSE_MEDIACTRL OFF)

	FetchContent_MakeAvailable(wxWidgets)
    set(wxWidgets_CONFIG_EXECUTABLE ${wxWidgets_BINARY_DIR}/wx-config)
    find_package(wxWidgets)
endif()

message(${wxWidgets_SOURCE_DIR}${wxWidgets_BINARY_DIR})

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