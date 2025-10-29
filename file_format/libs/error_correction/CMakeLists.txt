add_library(error_correction SHARED crc32.c)

target_link_libraries(error_correction PUBLIC common)

target_include_directories(error_correction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
