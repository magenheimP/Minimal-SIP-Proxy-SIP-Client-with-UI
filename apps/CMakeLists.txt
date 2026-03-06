# Placeholder for future applications

add_executable(sip_proxy sip_proxy_main.cpp)
target_link_libraries(sip_proxy PRIVATE proxy common networking)

add_executable(sip_client sip_client_main.cpp)
target_link_libraries(sip_client PRIVATE client common networking)
