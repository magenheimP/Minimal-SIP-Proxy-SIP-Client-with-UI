add_library(networking INTERFACE
        include/networking/udp_socket.h
        src/udp_socket.cpp
        include/networking/IUdpTransport.h
        include/networking/udp_transport.h
        src/udp_transport.cpp)

target_include_directories(networking
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(networking
        INTERFACE
        common
        project_options
)