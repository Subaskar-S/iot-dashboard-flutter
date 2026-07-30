# IoT Dashboard source tree
#
# Module subdirectories are added only when present so the build stays
# green while modules are implemented incrementally (see CLAUDE.md).

set(IOT_MODULES common core database network devices security data analytics api)
set(IOT_LINK_LIBS Threads::Threads)

foreach(module ${IOT_MODULES})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${module}")
        add_subdirectory(${module})
        # "network" is a container; its outputs are iot_mqtt, iot_http, etc.
        if(NOT "${module}" STREQUAL "network")
            list(PREPEND IOT_LINK_LIBS iot_${module})
        else()
            foreach(sub mqtt http websocket)
                if(TARGET iot_${sub})
                    list(PREPEND IOT_LINK_LIBS iot_${sub})
                endif()
            endforeach()
        endif()
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
