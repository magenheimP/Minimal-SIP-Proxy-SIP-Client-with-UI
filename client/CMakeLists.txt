add_library(client INTERFACE)

target_include_directories(client
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(client
        INTERFACE
        common
        networking
        project_options
)