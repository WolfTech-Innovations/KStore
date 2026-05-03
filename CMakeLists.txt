cmake_minimum_required(VERSION 3.16)

project(kstore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(ECM 5.80.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Network)
find_package(KF5 REQUIRED COMPONENTS Kirigami2 I18n CoreAddons)

add_executable(kstore
    src/main.cpp
    src/kstorebackend.cpp
    src/kstorebackend.h
    src/resources.qrc
)

target_link_libraries(kstore
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Network
    KF5::Kirigami2
    KF5::I18n
    KF5::CoreAddons
)

install(TARGETS kstore ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
