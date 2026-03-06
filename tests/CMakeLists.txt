add_executable(parser_tests
        parser_tests.cpp
)

target_link_libraries(parser_tests
        PRIVATE
        common
)
