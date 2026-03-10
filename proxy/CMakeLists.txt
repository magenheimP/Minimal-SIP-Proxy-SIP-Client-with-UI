add_library(proxy
        src/sip_parser.cpp
        src/registration_table.cpp
        src/register_handler.cpp)

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