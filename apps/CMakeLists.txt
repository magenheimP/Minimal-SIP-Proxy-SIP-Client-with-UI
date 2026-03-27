find_package(Qt6 COMPONENTS Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

add_executable(sip_proxy
        sip_proxy_main.cpp
)
target_link_libraries(sip_proxy
        PRIVATE
        proxy
        common
        networking
        metrics
)

add_executable(sip_client
        sip_client_main.cpp
)

set_target_properties(sip_client PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
)

target_link_libraries(sip_client
        PRIVATE
        client
        ui
        project_options
        Qt${QT_VERSION_MAJOR}::Widgets
)
