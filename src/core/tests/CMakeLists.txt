add_executable(iot_core_tests
    concepts_test.cpp
    interfaces_test.cpp
)

target_link_libraries(iot_core_tests PRIVATE
    iot_core
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(iot_core_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_core_tests)
