cmake_minimum_required(VERSION 3.16)

project(SIPTrainingProject VERSION 1.0 LANGUAGES CXX)

# Project-wide options & warnings targets
add_library(project_options INTERFACE)
add_library(project_warnings INTERFACE)

# Enforce C++20 (in addition to the global set, harmless)
target_compile_features(project_options INTERFACE cxx_std_20)

# Strict warnings
if (MSVC)
    target_compile_options(project_warnings INTERFACE /W4 /WX)
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -Werror)
endif()

# Enforce C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compiler warnings
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Enable testing
enable_testing()

# Subdirectories
add_subdirectory(common)
add_subdirectory(networking)
add_subdirectory(proxy)
add_subdirectory(client)
add_subdirectory(ui)
add_subdirectory(apps)
add_subdirectory(tests)