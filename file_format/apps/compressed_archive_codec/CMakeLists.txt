add_executable(compressed_archive_codec main.c coder.c decoder.c)

target_link_libraries(compressed_archive_codec PRIVATE
    arguments arithmetic common archive_builder archive_reader
)

target_include_directories(compressed_archive_codec PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
