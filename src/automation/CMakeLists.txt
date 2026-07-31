add_library(iot_automation STATIC
    condition_evaluator.cpp
    rule_repository.cpp
    rule_engine.cpp
)

target_include_directories(iot_automation PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_automation PUBLIC
    iot_core
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

target_compile_features(iot_automation PUBLIC cxx_std_23)

install(TARGETS iot_automation
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
