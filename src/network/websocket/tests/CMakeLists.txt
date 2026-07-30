add_executable(iot_websocket_tests
    ws_test.cpp
)

target_link_libraries(iot_websocket_tests PRIVATE
    iot_websocket
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(iot_websocket_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_websocket_tests)
