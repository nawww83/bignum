cmake_minimum_required(VERSION 3.20)

project(matrix_crypto
    VERSION 2.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Родительский каталог содержит header-only библиотеку bignum (u128.hpp, ubig.hpp).
# Если проект лежит в подпапке этого репозитория — путь по умолчанию корректен.
if(NOT DEFINED BIGNUM_INCLUDE_DIR)
    set(BIGNUM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

add_executable(matrix_crypto main.cpp)

target_include_directories(matrix_crypto PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BIGNUM_INCLUDE_DIR}
)

if(MSVC)
    target_compile_options(matrix_crypto PRIVATE /W4 /permissive-)
else()
    # -Wpedantic не используем: header-only библиотека bignum применяет __int128.
    target_compile_options(matrix_crypto PRIVATE -Wall -Wextra -O2)
endif()
