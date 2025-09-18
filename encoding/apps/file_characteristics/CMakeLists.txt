add_executable(file_characteristics main.c statistics.c summary.c)

target_link_libraries(file_characteristics PRIVATE
    arguments file common
)

target_include_directories(file_characteristics PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
