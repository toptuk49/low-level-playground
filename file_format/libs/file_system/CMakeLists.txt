add_library(file_system SHARED directory_tree.c file.c file_list.c)

target_link_libraries(file_system PUBLIC
    common path_utils
)

target_include_directories(file_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
