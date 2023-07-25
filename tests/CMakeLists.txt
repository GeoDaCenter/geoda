include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# Add test cpp file
set(TEST_SOURCE_FILES
    ${GEODA_SOURCE_FILES}
    test-webgl-map.cpp)

message(PROJECT_SOURCE_DIR="${BOOST_SRC_DIR}")

add_executable(runUnitTests ${TEST_SOURCE_FILES})

target_include_directories(runUnitTests PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(runUnitTests PUBLIC ${wxWidgets_INCLUDE_DIRS})
target_include_directories(runUnitTests PUBLIC
    ${gtest_SOURCE_DIR}/include
    ${gmock_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)

target_compile_definitions(runUnitTests PUBLIC ${wxWidgets_DEFINITIONS})

target_link_libraries(runUnitTests
    ${wxWidgets_LIBRARIES}
    gtest
    gtest_main)

add_test(runUnitTests runUnitTests)
