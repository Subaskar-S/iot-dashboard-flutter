cmake_minimum_required(VERSION 3.20)

# Suppress FindBoost deprecation warning (CMP0167) on CMake 3.27+
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(IoTDashboard VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Project configuration
# ============================================================================
# NOTE: std::expected (used throughout for Result<T> error handling) is only
# exposed by libc++ in C++23 mode, not C++20. C++23 is a superset of C++20
# feature-wise (concepts, ranges, coroutines all remain available), so the
# project targets C++23 to get std::expected without a third-party polyfill.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_ROOT_NAME "IoTDashboard")

# ============================================================================
# CMake modules
# ============================================================================
list(APPEND CMAKE_MODULE_PATH "${PROJECT_ROOT}/cmake")
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(functions)
include(CompilationFlags)

# ============================================================================
# Build options
# ============================================================================
option(BUILD_TESTING "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_MQTT "Enable MQTT support" ON)
option(ENABLE_MODBUS "Enable Modbus support" ON)
option(ENABLE_OPCUA "Enable OPC UA support" OFF)

# ============================================================================
# Detect platform
# ============================================================================
if(APPLE)
    set(BUILD_PLATFORM_NAME "OSX")
elseif(UNIX)
    set(BUILD_PLATFORM_NAME "Linux")
elseif(WIN32)
    set(BUILD_PLATFORM_NAME "Windows")
endif()

# ============================================================================
# Dependencies
# ============================================================================

# Threads
find_package(Threads REQUIRED)

# Boost (ASIO, Beast for HTTP/WebSocket)
# NOTE: boost_system is header-only since Boost 1.69 and was removed as an
# installable component in newer Boost releases, so only "thread" is linked.
find_package(Boost 1.75 REQUIRED COMPONENTS thread)

# JSON
find_package(nlohmann_json 3.10 REQUIRED)

# spdlog
find_package(spdlog REQUIRED)

# fmt
find_package(fmt REQUIRED)

# OpenSSL
find_package(OpenSSL REQUIRED)

# SQLite3
find_package(SQLite3 REQUIRED)

# GTest (for testing)
if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# MQTT (Paho C async library)
# Ubuntu libpaho-mqtt-dev ships a pkg-config file; macOS brew provides cmake config.
if(ENABLE_MQTT)
    find_package(eclipse-paho-mqtt-c QUIET)
    if(eclipse-paho-mqtt-c_FOUND)
        add_compile_definitions(IOT_HAS_MQTT)
        message(STATUS "Paho MQTT C found (cmake config)")
    else()
        # Fallback: try pkg-config (Ubuntu)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(PAHO_MQTT QUIET paho-mqtt3as)
        endif()
        if(PAHO_MQTT_FOUND)
            add_compile_definitions(IOT_HAS_MQTT)
            # Create an imported target so the source uses the same name everywhere.
            add_library(eclipse-paho-mqtt-c::paho-mqtt3as INTERFACE IMPORTED)
            target_include_directories(eclipse-paho-mqtt-c::paho-mqtt3as
                INTERFACE ${PAHO_MQTT_INCLUDE_DIRS})
            target_link_libraries(eclipse-paho-mqtt-c::paho-mqtt3as
                INTERFACE ${PAHO_MQTT_LIBRARIES})
            message(STATUS "Paho MQTT C found (pkg-config)")
        else()
            message(STATUS "Paho MQTT not found - MQTT support disabled")
            set(ENABLE_MQTT OFF)
        endif()
    endif()
endif()

# ============================================================================
# Project include directories
# ============================================================================
include_directories(${PROJECT_ROOT}/src)

# ============================================================================
# Source tree
# ============================================================================
add_subdirectory(src)

# ============================================================================
# Tests
# ============================================================================
if(BUILD_TESTING AND IS_DIRECTORY "${PROJECT_ROOT}/test")
    add_subdirectory(test)
endif()

# ============================================================================
# Examples
# ============================================================================
if(BUILD_EXAMPLES AND IS_DIRECTORY "${PROJECT_ROOT}/example")
    add_subdirectory(example)
endif()

# ============================================================================
# Installation
# ============================================================================
install(TARGETS iot-dashboard
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${PROJECT_ROOT}/src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iot
    FILES_MATCHING PATTERN "*.hpp")

# Package config
install(EXPORT IoTDashboardTargets
    FILE IoTDashboardTargets.cmake
    NAMESPACE iot::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IoTDashboard)

configure_package_config_file(
    "${PROJECT_ROOT}/cmake/IoTDashboardConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/IoTDashboardConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IoTDashboard)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/IoTDashboardConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IoTDashboard)
