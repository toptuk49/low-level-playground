add_library(archive_reader SHARED compressed_archive_reader.c)

target_link_libraries(archive_reader PUBLIC 
    common archive_header file_table path_utils
)

target_include_directories(archive_reader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
