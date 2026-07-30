add_executable(iot_http_tests
    http_test.cpp
)

target_link_libraries(iot_http_tests PRIVATE
    iot_http
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(iot_http_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_http_tests)
