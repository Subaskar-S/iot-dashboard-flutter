add_library(iot_devices STATIC
    device_manager.cpp
    heartbeat_monitor.cpp
)

target_include_directories(iot_devices PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_devices PUBLIC
    iot_core
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

target_compile_features(iot_devices PUBLIC cxx_std_23)

install(TARGETS iot_devices
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
