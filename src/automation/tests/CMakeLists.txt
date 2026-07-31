add_executable(iot_automation_tests
    condition_evaluator_test.cpp
    rule_repository_test.cpp
    rule_engine_test.cpp
)

target_link_libraries(iot_automation_tests PRIVATE
    iot_automation
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(iot_automation_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(iot_automation_tests)
