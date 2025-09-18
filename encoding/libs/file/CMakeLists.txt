add_library(file SHARED file.c)
target_link_libraries(file PUBLIC common)
target_include_directories(file PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
