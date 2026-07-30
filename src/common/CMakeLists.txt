add_library(iot_common STATIC
    error.cpp
    logging.cpp
)

target_include_directories(iot_common PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_common PUBLIC
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    Boost::boost
)

target_compile_features(iot_common PUBLIC cxx_std_23)

# Install
install(TARGETS iot_common
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
