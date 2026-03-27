add_library(metrics
        src/metrics_collector.cpp
        src/metrics_server.cpp
)

target_include_directories(metrics
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(metrics
        PUBLIC
        project_options
)