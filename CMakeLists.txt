cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)

project(logarithm)
add_executable(logarithm)

option(ENABLE_ASAN OFF)

target_compile_features(logarithm PUBLIC cxx_std_23)

target_include_directories(logarithm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_sources(logarithm PRIVATE
    src/main.cpp
)

target_compile_options(logarithm PRIVATE
$<$<CONFIG:Debug>:
    -Wall
    -Wextra
    -Wpedantic
    -DDEBUG
    -Wmissing-declarations
    -Wcast-align
    -Wshadow
    -Wsign-conversion>
)

if(ENABLE_ASAN)
    target_compile_options(logarithm PRIVATE $<$<CONFIG:Debug>: -fsanitize=address>)
    target_link_options(logarithm PRIVATE $<$<CONFIG:Debug>: -fsanitize=address>)
endif()