# IoT Dashboard source tree
#
# Module subdirectories are added only when present so the build stays
# green while modules are implemented incrementally (see CLAUDE.md).

set(IOT_MODULES common core security device data analytics api)
set(IOT_LINK_LIBS Threads::Threads)

foreach(module ${IOT_MODULES})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${module}")
        add_subdirectory(${module})
        list(PREPEND IOT_LINK_LIBS iot_${module})
    endif()
endforeach()

# Main executable
add_executable(iot-dashboard
    main.cpp
)

target_link_libraries(iot-dashboard PRIVATE
    ${IOT_LINK_LIBS}
)

target_compile_features(iot-dashboard PRIVATE cxx_std_23)

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
