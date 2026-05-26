cmake_minimum_required(VERSION 4.0)

# Enable C++20 modules
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
#set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "gcc")

# Set C++ standard for C++20 modules
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(gstd LANGUAGES CXX)

if (MSVC)
    add_compile_options(/utf-8)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add the main library
add_subdirectory(s.gstd)
