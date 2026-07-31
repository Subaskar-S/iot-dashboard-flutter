add_executable(iot_api_tests
    api_test.cpp
)

target_link_libraries(iot_api_tests PRIVATE
    iot_api
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(iot_api_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_api_tests)
