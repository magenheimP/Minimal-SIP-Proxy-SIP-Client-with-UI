add_library(common STATIC
        src/sip_message.cpp
        src/logger.cpp)

# Make headers visible to any target linking with common
target_include_directories(common
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add project-wide options and warnings
target_link_libraries(common
        PUBLIC project_options project_warnings
)