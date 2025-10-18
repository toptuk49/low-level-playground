add_executable(raw_archive_codec main.c coder.c decoder.c)

target_link_libraries(raw_archive_codec PRIVATE
    arguments file_system common archive_format
)

target_include_directories(raw_archive_codec PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
