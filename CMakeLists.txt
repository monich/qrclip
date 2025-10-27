cmake_minimum_required(VERSION 3.0)
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
    qrclip.desktop
    qrclip.qrc
    qrclip_app.cpp
    qrclip_app.h
    qrclip_config.cpp
    qrclip_config.h
    qrclip_debug.h
    qrclip_widget.cpp
    qrclip_widget.h
    qrclip_window.cpp
    qrclip_window.h)

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

install(TARGETS qrclip DESTINATION /usr/bin)
install(FILES qrclip.svg DESTINATION /usr/share/pixmaps)
install(FILES qrclip.desktop DESTINATION /usr/share/applications)

set(CPACK_GENERATOR DEB)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Slava Monich <slava@monich.com>")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)
set(CPACK_PACKAGE_VERSION 1.0.3)
set(CPACK_PACKAGE_VENDOR "Slava Monich")
set(CPACK_PACKAGE_DESCRIPTION "Show clipboard text as a QR code")
set(CPACK_PACKAGE_ICON "qrclip.svg")
include(CPack)
