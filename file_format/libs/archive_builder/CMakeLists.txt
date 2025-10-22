add_library(archive_builder SHARED compressed_archive_builder.c)

target_link_libraries(archive_builder PUBLIC 
    common archive_header file_table path_utils
)

target_include_directories(archive_builder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
