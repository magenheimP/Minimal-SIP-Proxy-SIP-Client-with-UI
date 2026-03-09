add_library(networking INTERFACE
        include/networking/udp_socket.hpp
        src/udp_socket.cpp
        include/networking/IUdpTransport.hpp
        include/networking/udp_transport.hpp
        src/udp_transport.cpp
)

target_include_directories(networking
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(networking
        INTERFACE
        common
        project_options
)