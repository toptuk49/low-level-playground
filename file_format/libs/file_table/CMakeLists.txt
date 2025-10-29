add_library(file_table SHARED file_table.c)

target_link_libraries(file_table PUBLIC
  common file_system path_utils
)

target_include_directories(file_table PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
