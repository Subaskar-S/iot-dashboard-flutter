add_library(iot_database STATIC
    sqlite_connection.cpp
    sqlite_statement.cpp
    migration_manager.cpp
    device_repository.cpp
    sensor_repository.cpp
)

target_include_directories(iot_database PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_database PUBLIC
    iot_core
    SQLite3::SQLite3
    spdlog::spdlog
)

target_compile_features(iot_database PUBLIC cxx_std_23)

install(TARGETS iot_database
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
