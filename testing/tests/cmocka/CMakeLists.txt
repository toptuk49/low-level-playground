add_executable(cmocka_tests
    calculations.test.c
)

target_link_libraries(cmocka_tests PUBLIC
    cmocka::cmocka
    calculations
)

target_include_directories(cmocka_tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(cmocka_tests PRIVATE
    HAVE_CMOCKA_H
)
