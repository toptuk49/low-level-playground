add_executable(file_characteristics file_characteristics.c statistics.c summary.c)
target_link_libraries(file_characteristics arguments file)
target_include_directories(file_characteristics PRIVATE 
    ../../libs/arguments
    ../../libs/file
)
