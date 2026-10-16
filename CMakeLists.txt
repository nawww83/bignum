cmake_minimum_required(VERSION 3.20)

project(bignum_project
    VERSION 1.0.0
    LANGUAGES CXX
)

# Обязательный стандарт C++20 (требуется для concepts, spaceship operator <=>, <bit>)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------
# Header-only библиотека с заголовочными файлами
# ---------------------------------------------------------
add_library(bignum INTERFACE)

target_include_directories(bignum INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Исполняемый файл для тестов и бенчмарков
# ---------------------------------------------------------
add_executable(bignum_tests
    main.cpp
    tests/u128_test.cpp
    tests/i128_test.cpp
    tests/ubig_test.cpp
    tests/benchmark.cpp
)

target_link_libraries(bignum_tests PRIVATE
    bignum
)

# Добавляем каталог tests в пути поиска инклюдов при необходимости
target_include_directories(bignum_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# Базовые флаги предупреждений и оптимизаций компилятора
if(MSVC)
    target_compile_options(bignum_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(bignum_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
