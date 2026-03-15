file(GLOB_RECURSE GSTD_SOURCES "src/*.cppm")
file(GLOB_RECURSE UNITTEST_SOURCES "unittest/*.cpp")

# Remove platform-specific files that don't match the current OS
if(WIN32)
    file(GLOB_RECURSE LINUX_SOURCES "src/os/linux/*.cppm")
    list(REMOVE_ITEM GSTD_SOURCES ${LINUX_SOURCES})
else()
    file(GLOB_RECURSE WINDOWS_SOURCES "src/os/windows/*.cppm")
    list(REMOVE_ITEM GSTD_SOURCES ${WINDOWS_SOURCES})
endif()

# Create the main library with modules
add_library(gstd)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Add module files with proper dependency handling
target_sources(gstd
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES 
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
        FILES ${GSTD_SOURCES}
)

target_compile_features(gstd PUBLIC cxx_std_23)
target_link_libraries(gstd PUBLIC Threads::Threads)
target_include_directories(gstd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Create the test executable
add_executable(gstd_tests)
target_sources(gstd_tests PRIVATE ${UNITTEST_SOURCES})
target_compile_features(gstd_tests PRIVATE cxx_std_23)
target_link_libraries(gstd_tests PRIVATE gstd Threads::Threads)

# For MSVC, ensure proper module scanning for tests
if(MSVC)
    target_compile_options(gstd       PRIVATE /EHsc)
    target_compile_options(gstd_tests PRIVATE /EHsc)
endif()

# Enable testing
enable_testing()
add_test(NAME gstd_tests COMMAND gstd_tests)
