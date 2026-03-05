add_library(proxy INTERFACE)

target_include_directories(proxy
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(proxy
        INTERFACE
        common
        networking
        project_options
)