add_executable(substrings substrings.c)
target_link_libraries(substrings arguments file)
target_include_directories(substrings PRIVATE 
    ../../libs/arguments
    ../../libs/file
)
