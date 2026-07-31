add_library(iot_api STATIC
    application.cpp
    middleware/auth_middleware.cpp
    controllers/health_controller.cpp
    controllers/auth_controller.cpp
    controllers/device_controller.cpp
    controllers/sensor_controller.cpp
    controllers/automation_controller.cpp
    controllers/metrics_controller.cpp
)

target_include_directories(iot_api PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_api PUBLIC
    iot_core
    iot_database
    iot_devices
    iot_automation
    iot_security
    iot_mqtt
    iot_http
    iot_websocket
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

target_compile_features(iot_api PUBLIC cxx_std_23)

install(TARGETS iot_api
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
