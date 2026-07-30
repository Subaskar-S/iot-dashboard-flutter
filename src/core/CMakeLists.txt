# iot_core: domain concepts and repository/service interfaces (ports).
# Header-only: no .cpp files, no dependencies beyond common and nlohmann_json.

add_library(iot_core INTERFACE)

target_include_directories(iot_core INTERFACE
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_core INTERFACE
    iot_common
    nlohmann_json::nlohmann_json
)

target_compile_features(iot_core INTERFACE cxx_std_23)

install(TARGETS iot_core
    EXPORT IoTDashboardTargets)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
