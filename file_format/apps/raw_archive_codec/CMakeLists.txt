add_executable(raw_archive_codec main.c coder.c decoder.c)

target_link_libraries(raw_archive_codec PRIVATE
    archive_format common
)

target_include_directories(raw_archive_codec PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
