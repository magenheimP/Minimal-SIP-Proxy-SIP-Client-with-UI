add_library(proxy
        src/sip_parser.cpp
        src/registration_table.cpp
        src/register_handler.cpp
        include/proxy/call_state.hpp
        src/call_state.cpp
        include/proxy/call_state_machine.hpp
        src/call_state_machine.cpp
        include/proxy/call_session.hpp
        src/call_session.cpp)

target_include_directories(proxy
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(proxy
        PUBLIC
        common
        networking
        project_options
        project_warnings
)