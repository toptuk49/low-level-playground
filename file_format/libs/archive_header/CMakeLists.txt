add_library(archive_header SHARED raw_archive_header.c compressed_archive_header.c)

target_link_libraries(archive_header PUBLIC 
    common file_system error_correction
)

target_include_directories(archive_header PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
