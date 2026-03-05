add_library(networking INTERFACE)

target_include_directories(networking
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(networking
        INTERFACE
        common
        project_options
)