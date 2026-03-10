add_library(common STATIC
        src/sip_message.cpp)
add_library(common_headers INTERFACE
        include/common/thread_safe_queue.hpp)

# Make headers visible to any target linking with common
target_include_directories(common_headers
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common
        PUBLIC common_headers project_options project_warnings
)
