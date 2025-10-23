add_library(calculations SHARED calculations.c)

target_include_directories(calculations PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
