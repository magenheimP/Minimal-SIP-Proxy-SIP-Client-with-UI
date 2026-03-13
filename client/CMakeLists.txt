add_library(client
        src/sip_client.cpp
        include/client/input_handler.hpp
        src/sip_register_handler.cpp
)

target_include_directories(client
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(client
        PUBLIC
        common
        networking
        project_options
)

add_executable(client_cli
        src/client_cli.cpp
)

target_link_libraries(client_cli
        client
        common
        networking
)