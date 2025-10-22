add_library(path_utils SHARED path_utils.c)

target_link_libraries(path_utils PUBLIC common)

target_include_directories(path_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
