project(tests LANGUAGES CXX)

add_executable(tests calculations.test.cc)

target_link_libraries(tests PUBLIC
    gtest::gtest
    calculations
)

target_include_directories(tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)
