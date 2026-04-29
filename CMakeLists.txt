cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)

project(logarithm)
add_executable(logarithm)
add_executable(verification)
add_executable(tests)

option(ENABLE_ASAN OFF)

target_compile_features(logarithm PUBLIC cxx_std_23)

target_include_directories(logarithm PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
target_sources(logarithm PRIVATE
    src/main.cpp
    src/loglib.cpp
)

target_include_directories(verification PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/verification
)
target_sources(verification PRIVATE
    verification/verification.cpp
    src/loglib.cpp
)

target_include_directories(tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/verification
    ${CMAKE_SOURCE_DIR}/../Benchmark_task/inc
)
target_sources(tests PRIVATE
    verification/tests.cpp
    src/loglib.cpp
)

foreach(target logarithm verification tests)
target_compile_options(${target} PRIVATE
        -O3
        -mavx2
        -mfma
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
endforeach()

if(ENABLE_ASAN)
    foreach(target logarithm verification tests)
        target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>: -fsanitize=address>)
        target_link_options(${target} PRIVATE $<$<CONFIG:Debug>: -fsanitize=address>)
    endforeach()
endif()