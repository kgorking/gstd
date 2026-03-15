cmake_minimum_required(VERSION 3.28)

# Enable C++20 modules
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
#set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "gcc")

# Set C++ standard for C++20 modules
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(gstd LANGUAGES CXX)

# Find required packages
find_package(Threads REQUIRED)

if (MSVC)
    add_compile_options(/utf-8)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add the main library
add_subdirectory(s.gstd)
