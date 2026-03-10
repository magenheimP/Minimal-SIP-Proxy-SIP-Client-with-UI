add_library(networking
        src/thread_pool.cpp
        include/networking/thread_pool.hpp
        include/networking/udp_socket.hpp
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