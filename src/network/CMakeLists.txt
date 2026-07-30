# Network module — submodules added only when their directory exists.

set(IOT_NETWORK_SUBMODULES mqtt http websocket)

foreach(sub ${IOT_NETWORK_SUBMODULES})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${sub}")
        add_subdirectory(${sub})
    endif()
endforeach()
