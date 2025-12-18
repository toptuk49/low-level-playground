add_library(archive_builder SHARED raw_archive_builder.c compressed_archive_builder.c)

target_link_libraries(archive_builder PUBLIC 
    common
    archive_header
    arithmetic
    file_table
    huffman
    lz77
    lz78
    path_utils
    rle
    shannon
)

target_include_directories(archive_builder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
