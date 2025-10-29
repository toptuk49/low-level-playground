add_executable(raw_archive_codec main.c coder.c decoder.c)

target_link_libraries(raw_archive_codec PRIVATE
    arguments common archive_builder archive_reader
)

target_include_directories(raw_archive_codec PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
