add_library(archive_reader SHARED raw_archive_reader.c compressed_archive_reader.c)

target_link_libraries(archive_reader PUBLIC 
    common archive_header arithmetic file_table path_utils huffman shannon
)

target_include_directories(archive_reader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
