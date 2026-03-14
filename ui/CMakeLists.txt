add_library(ui INTERFACE
        main.cpp)

target_link_libraries(ui
        INTERFACE
        client
        project_options
)