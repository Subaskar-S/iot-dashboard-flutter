add_executable(iot_mqtt_tests
    mqtt_client_test.cpp
)

target_link_libraries(iot_mqtt_tests PRIVATE
    iot_mqtt
    iot_http
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(iot_mqtt_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_mqtt_tests)
