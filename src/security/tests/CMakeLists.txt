add_executable(iot_security_tests
    password_hasher_test.cpp
    jwt_handler_test.cpp
    access_control_test.cpp
    authentication_service_test.cpp
)

target_link_libraries(iot_security_tests PRIVATE
    iot_security
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(iot_security_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_security_tests)
