cmake_minimum_required(VERSION 3.20)

project(kstore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# Find Qt (attempt 6, fallback to 5 for sandbox)
find_package(Qt6 COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Network)
if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found, falling back to Qt5")
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Network)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# Find KF components (attempt 6, fallback to 5)
find_package(KF6 COMPONENTS Kirigami I18n CoreAddons)
if(NOT KF6_FOUND)
    message(STATUS "KF6 not found, falling back to KF5")
    find_package(KF5 REQUIRED COMPONENTS Kirigami2 I18n CoreAddons)
    set(KDE_KIRIGAMI KF5::Kirigami2)
    set(KDE_I18N KF5::I18n)
    set(KDE_COREADDONS KF5::CoreAddons)
else()
    set(KDE_KIRIGAMI KF6::Kirigami)
    set(KDE_I18N KF6::I18n)
    set(KDE_COREADDONS KF6::CoreAddons)
endif()

add_executable(kstore
    src/main.cpp
    src/kstorebackend.cpp
    src/kstorebackend.h
    src/resources.qrc
)

target_link_libraries(kstore
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${KDE_KIRIGAMI}
    ${KDE_I18N}
    ${KDE_COREADDONS}
)

install(TARGETS kstore ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
