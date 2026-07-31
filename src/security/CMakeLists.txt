add_library(iot_security STATIC
    password_hasher.cpp
    jwt_handler.cpp
    access_control.cpp
    authentication_service.cpp
)

target_include_directories(iot_security PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_security PUBLIC
    iot_core
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Point to Homebrew OpenSSL headers on macOS.
if(APPLE)
    target_include_directories(iot_security PRIVATE
        /opt/homebrew/opt/openssl@3/include
    )
endif()

target_compile_features(iot_security PUBLIC cxx_std_23)

install(TARGETS iot_security
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
