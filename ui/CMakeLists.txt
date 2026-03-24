find_package(Qt6 COMPONENTS Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

add_library(ui
        src/main_window.cpp
        src/sip_qt_app.cpp
        src/header_injection_widget.cpp
        include/ui/main_window.hpp
        include/ui/sip_qt_app.hpp
        include/ui/header_injection_widget.hpp
)

set_target_properties(ui PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
)

target_include_directories(ui
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ui
        PUBLIC
        client
        project_options
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
)