qt_add_library(TaisyncModule STATIC)

qt_add_qml_module(TaisyncModule
    URI QGroundControl.Taisync
    VERSION 1.0
    RESOURCE_PREFIX /qml
    SOURCES
        TaisyncStatusManager.cpp
        TaisyncStatusManager.h
    NO_PLUGIN
)

target_include_directories(TaisyncModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(TaisyncModule
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Qml
)