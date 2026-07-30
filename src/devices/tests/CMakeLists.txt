add_executable(iot_devices_tests
    device_manager_test.cpp
    heartbeat_monitor_test.cpp
)

target_link_libraries(iot_devices_tests PRIVATE
    iot_devices
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

target_compile_features(iot_devices_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_devices_tests)
