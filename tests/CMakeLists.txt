include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# Add test cpp file
set(TEST_SOURCE_FILES
    ${GEODA_SOURCE_FILES}
    test-boost-utils.cpp
    test-geom-utils.cpp
    test-webgl-map.cpp)

add_executable(runUnitTests ${TEST_SOURCE_FILES})

target_include_directories(runUnitTests PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIR}
    ${gtest_SOURCE_DIR}/include
    ${gmock_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)

target_compile_definitions(runUnitTests PUBLIC ${wxWidgets_DEFINITIONS})

target_link_libraries(runUnitTests
    ${wxWidgets_LIBRARIES}
    GDAL::GDAL
    gtest
    gtest_main)

add_test(runUnitTests runUnitTests)
file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
