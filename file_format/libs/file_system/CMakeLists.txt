add_library(file_system SHARED file.c file_system.c)

target_link_libraries(file_system PUBLIC common)

target_include_directories(file_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
