add_executable(unity_tests
    calculations.test.c
)

target_link_libraries(unity_tests PUBLIC
    unity::unity
    calculations
)

target_include_directories(unity_tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
