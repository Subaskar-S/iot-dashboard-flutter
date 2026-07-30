# IoT Dashboard source tree

# Module subdirectories
add_subdirectory(common)
add_subdirectory(security)
add_subdirectory(device)
add_subdirectory(data)
add_subdirectory(analytics)
add_subdirectory(api)

# Main executable
add_executable(iot-dashboard
    main.cpp
)

target_link_libraries(iot-dashboard PRIVATE
    iot_api
    iot_device
    iot_data
    iot_analytics
    iot_security
    iot_common
    Threads::Threads
)

target_compile_features(iot-dashboard PRIVATE cxx_std_20)

# Platform-specific linking
if(APPLE)
    target_link_options(iot-dashboard PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(iot-dashboard PRIVATE dl)
endif()

# Install
install(TARGETS iot-dashboard
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
