add_library(ui INTERFACE)

target_link_libraries(ui
        INTERFACE
        client
        project_options
)