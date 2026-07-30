add_library(iot_websocket STATIC
    ws_session.cpp
    ws_server.cpp
)

target_include_directories(iot_websocket PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_websocket PUBLIC
    iot_core
    Boost::headers
    Boost::thread
    spdlog::spdlog
    Threads::Threads
)

target_compile_features(iot_websocket PUBLIC cxx_std_23)

target_compile_definitions(iot_websocket PUBLIC
    BOOST_BEAST_USE_STD_STRING_VIEW
    BOOST_ASIO_NO_DEPRECATED
)

install(TARGETS iot_websocket
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
