cmake_minimum_required(VERSION 3.5)

project(qrclip)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

pkg_check_modules(LIBQRENCODE REQUIRED libqrencode)

set(SRC
    main.cpp
    qrclip.qrc
    qrclip_widget.cpp
    qrclip_widget.h)

add_executable(qrclip ${SRC})

target_sources(qrclip PRIVATE
    LICENSE
    README.md)

target_compile_definitions(qrclip PRIVATE
    $<$<CONFIG:Debug>:QRCLIP_DEBUG=1>)

target_compile_options(qrclip PUBLIC
    ${LIBQRENCODE_CFLAGS_OTHER})

target_include_directories(qrclip PUBLIC
    ${LIBQRENCODE_INCLUDE_DIRS})

target_link_libraries(qrclip
    ${LIBQRENCODE_LIBRARIES}
    Qt${QT_VERSION_MAJOR}::Widgets)
