add_library(iot_http STATIC
    http_types.cpp
    http_router.cpp
    http_session.cpp
    http_server.cpp
)

target_include_directories(iot_http PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_http PUBLIC
    iot_core
    Boost::headers
    Boost::thread
    spdlog::spdlog
    Threads::Threads
)

target_compile_features(iot_http PUBLIC cxx_std_23)

# Beast requires these definitions.
target_compile_definitions(iot_http PUBLIC
    BOOST_BEAST_USE_STD_STRING_VIEW
    BOOST_ASIO_NO_DEPRECATED
)

install(TARGETS iot_http
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
