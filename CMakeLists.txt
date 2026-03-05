cmake_minimum_required(VERSION 3.16)

project(SIPTrainingProject VERSION 1.0 LANGUAGES CXX)

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