cmake_minimum_required(VERSION 3.27...3.27)

project(geoda
    DESCRIPTION "GeoDa: An introduction to spatial data analysis"
    LANGUAGES CXX
    VERSION 1.24.0
)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_POLICY_DEFAULT_CMP0048 "NEW")
set(CMAKE_POLICY_DEFAULT_CMP0063 "NEW")
# sources
include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(src)

# dependencies
include(FetchContent)
set(FETCHCONTENT_QUIET NO)
# boost
find_package(Boost 1.75.0)
if(NOT Boost_FOUND)
    FetchContent_Declare(boost
        URL https://boostorg.jfrog.io/artifactory/main/release/1.75.0/source/boost_1_75_0.tar.bz2
    )
    FetchContent_MakeAvailable(boost)
    FetchContent_GetProperties(boost SOURCE_DIR Boost_INCLUDE_DIRS)
endif()
# wxWidgets
set(wxWidgets_CONFIG_EXECUTABLE ${wxWidgets_BINARY_DIR}/wx-config)
find_package(wxWidgets)
if(NOT wxWidgets_FOUND)
    FetchContent_Declare(
		wxWidgets
		URL https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.2.1/wxWidgets-3.2.2.1.tar.bz2
	)
    if(APPLE)
	endif()
    set(wxBUILD_INSTALL ON)
    set(wxBUILD_SHARED OFF CACHE BOOL "Build wx libraries as shared libs")
    set(wxBUILD_MONOLITHIC ON CACHE BOOL "Build a single library")
    set(wxBUILD_PRECOMP ON CACHE BOOL "Use precompiled headers")
    set(wxUSE_GLCANVAS ON CACHE BOOL "Use OpenGL")
    set(wxUSE_POSTSCRIPT ON CACHE BOOL "Use postscript for pdf")
    set(wxUSE_TEXTFILE ON)
    set(wxUSE_WEBVIEW ON)
    set(wxUSE_LIBLZMA OFF)
    set(wxUSE_MEDIACTRL OFF)
	set(wxUSE_LIBSDL OFF CACHE BOOL "Use SDL for wxSound implementation.")

	FetchContent_MakeAvailable(wxWidgets)
    set(wxWidgets_CONFIG_EXECUTABLE ${wxWidgets_BINARY_DIR}/wx-config)
    find_package(wxWidgets)
endif()
# GDAL
find_package(GDAL REQUIRED)
# JSON Spirit
FetchContent_Declare(
	json_spirit
	URL https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip
)
#set(Boost_NO_BOOST_CMAKE TRUE)
#set(BOOST_ROOT ${Boost_INCLUDE_DIRS})
#set(JSON_SPIRIT_VALUE_ENABLED)
FetchContent_MakeAvailable(json_spirit)

message(aaa"${GDAL_LIBRARIES}")

set(CMAKE_CXX_STANDARD 17)
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