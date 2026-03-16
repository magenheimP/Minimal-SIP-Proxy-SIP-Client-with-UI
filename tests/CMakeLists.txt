add_executable(parser_tests
        parser_tests.cpp
)

target_link_libraries(parser_tests
        PRIVATE
        proxy
        common
)

add_executable(register_tests
        register_tests.cpp
)

target_link_libraries(register_tests
        PRIVATE
        proxy
        common
)

add_executable(thread_pool_tests
        thread_pool_tests.cpp
)

target_link_libraries(thread_pool_tests
        PRIVATE
        proxy
        common
)