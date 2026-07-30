add_library(iot_mqtt STATIC
    mqtt_client.cpp
)

target_include_directories(iot_mqtt PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_mqtt PUBLIC
    iot_core
    eclipse-paho-mqtt-c::paho-mqtt3as   # async + TLS
    spdlog::spdlog
)

target_compile_features(iot_mqtt PUBLIC cxx_std_23)

install(TARGETS iot_mqtt
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
