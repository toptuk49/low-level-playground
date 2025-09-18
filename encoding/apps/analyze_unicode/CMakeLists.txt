add_executable(analyze_unicode main.c)

target_link_libraries(analyze_unicode PRIVATE
    arguments file unicode common
)

target_include_directories(analyze_unicode PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
