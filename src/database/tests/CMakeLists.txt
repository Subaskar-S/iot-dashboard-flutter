add_executable(iot_database_tests
    database_test.cpp
)

target_link_libraries(iot_database_tests PRIVATE
    iot_database
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(iot_database_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_database_tests)
