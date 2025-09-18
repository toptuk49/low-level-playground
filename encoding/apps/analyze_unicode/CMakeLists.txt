add_executable(analyze_unicode analyze_unicode.c)
target_link_libraries(analyze_unicode arguments unicode file)
target_include_directories(analyze_unicode PRIVATE 
    ../../libs/arguments
    ../../libs/file
    ../../libs/unicode
)
