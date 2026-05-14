cmake_minimum_required(VERSION 3.25)
project(PriceTracker CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Boost REQUIRED)

# ── Solution library ───────────────────────────────────────────────────────────
add_library(order_book STATIC src/solution.cpp)
target_include_directories(order_book PUBLIC src)
target_link_libraries(order_book PUBLIC Boost::headers)
target_compile_options(order_book PRIVATE -march=native)

# ── Certified benchmark ────────────────────────────────────────────────────────
add_executable(benchmark benchmark/benchmark.cpp)
target_include_directories(benchmark PRIVATE benchmark)
target_link_libraries(benchmark PRIVATE order_book)
target_compile_options(benchmark PRIVATE -march=native)

# ── Comparison benchmark ───────────────────────────────────────────────────────
add_executable(compare benchmark/compare.cpp)
target_include_directories(compare PRIVATE benchmark)
target_link_libraries(compare PRIVATE order_book)
target_compile_options(compare PRIVATE -march=native)
