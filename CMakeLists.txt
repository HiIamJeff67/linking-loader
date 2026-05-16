cmake_minimum_required(VERSION 3.16)
project(linking_loader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(linking_loader
    main.cpp
    linking_loader.cpp
    passers.cpp
    util.cpp
)

target_compile_options(linking_loader PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_same_result
        test/test_same_result.cpp
    )

    target_compile_options(test_same_result PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    add_test(
        NAME compare_python_and_cpp_loader
        COMMAND test_same_result $<TARGET_FILE:linking_loader> ${CMAKE_SOURCE_DIR}
    )
endif()
