add_library(shannon SHARED shannon.c)

target_link_libraries(shannon PUBLIC common)

target_include_directories(shannon PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
