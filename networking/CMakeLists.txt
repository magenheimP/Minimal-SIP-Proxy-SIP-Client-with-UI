add_library(networking
        src/udp_socket.cpp
        src/udp_transport.cpp
)

target_include_directories(networking
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(networking
        PUBLIC
        common
        project_options
)