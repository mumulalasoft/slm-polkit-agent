cmake_minimum_required(VERSION 3.22)
project(slm-polkit-agent VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick DBus)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POLKIT_AGENT REQUIRED polkit-agent-1)
pkg_check_modules(POLKIT_GOBJECT REQUIRED polkit-gobject-1)

include(cmake/Sources.cmake)

qt_add_executable(slm-polkit-agent ${AGENT_SOURCES})

qt_add_qml_module(slm-polkit-agent
    URI SlmPolkitAgent
    VERSION 1.0
    QML_FILES ${AGENT_QML_FILES}
    RESOURCE_PREFIX /qt/qml
)

target_include_directories(slm-polkit-agent PRIVATE
    src
    ${POLKIT_AGENT_INCLUDE_DIRS}
    ${POLKIT_GOBJECT_INCLUDE_DIRS}
)

target_link_libraries(slm-polkit-agent PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::DBus
    ${POLKIT_AGENT_LIBRARIES}
    ${POLKIT_GOBJECT_LIBRARIES}
)

target_compile_options(slm-polkit-agent PRIVATE
    ${POLKIT_AGENT_CFLAGS_OTHER}
    ${POLKIT_GOBJECT_CFLAGS_OTHER}
)

install(TARGETS slm-polkit-agent DESTINATION libexec)
install(FILES systemd/slm-polkit-agent.service
    DESTINATION lib/systemd/user)
