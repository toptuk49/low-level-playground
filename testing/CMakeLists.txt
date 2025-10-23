cmake_minimum_required(VERSION 3.10)
project(encoding C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GTest REQUIRED)
find_package(unity REQUIRED)
find_package(cmocka REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
find_program(CLANG_FORMAT_EXE NAMES clang-format)

if(CLANG_TIDY_EXE)
    set(CMAKE_C_CLANG_TIDY 
        ${CLANG_TIDY_EXE} 
        -extra-arg=-Wno-unknown-warning-option
        --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
    )
endif()

if(CLANG_FORMAT_EXE)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${ALL_SOURCE_FILES}
        COMMENT "Formatting all source files"
    )

    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror --style=file ${ALL_SOURCE_FILES}
        COMMENT "Checking code formatting"
    )
endif()
