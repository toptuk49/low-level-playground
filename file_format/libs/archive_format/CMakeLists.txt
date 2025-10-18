add_library(archive_format SHARED archive_format.c compressed_archive_format.c)

target_link_libraries(archive_format PUBLIC 
    file_system common
)

target_include_directories(archive_format PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
