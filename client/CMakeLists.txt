add_library(client
        src/sip_client.cpp
        src/sip_message_factory.cpp
        src/sip_receive_handler.cpp
        src/sip_state_manager.cpp
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
        src/main.cpp
        src/cli_handler.cpp
)

target_link_libraries(client_cli
        client
        common
        networking
)