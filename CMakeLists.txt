cmake_minimum_required(VERSION 3.16)
project(linking_loader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(linking_loader
    main.cpp
    linking_loader.cpp
)

target_compile_options(linking_loader PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
