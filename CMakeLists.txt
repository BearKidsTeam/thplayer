cmake_minimum_required(VERSION 3.16.0)
project(thplayer CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)
option(WITH_OPENMP OFF)
add_subdirectory(thtk EXCLUDE_FROM_ALL)

#they assumed their project is the root project...
target_include_directories(thtk PUBLIC thtk/)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(ICU COMPONENTS uc i18n REQUIRED)
find_package(ECM 5.83.0 NO_MODULE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(CheckTypeSize)

if(ECM_FOUND)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    include(ECMAddAppIcon)
endif()

SET(CMAKE_EXTRA_INCLUDE_FILES "filesystem")
check_type_size("std::filesystem::path::value_type" PATH_VALSIZE LANGUAGE CXX)
SET(CMAKE_EXTRA_INCLUDE_FILES)

add_compile_definitions(PATH_VALSIZE=${PATH_VALSIZE})

add_executable(thplayer
    main.cpp
    loopedpcmstreamer.cpp loopedpcmstreamer.hpp
    mainwindow.cpp mainwindow.hpp
    outputselectiondialog.cpp outputselectiondialog.hpp
    songlist.cpp songlist.hpp
    thdatwrapper.cpp thdatwrapper.hpp

    mainwindow.ui
    outputselectiondialog.ui

    res.qrc
)

if(ECM_FOUND)
    ecm_add_app_icon(thplayer
    ICONS
        assets/256-thplayer.png
        assets/thplayer.svg
    )
endif()

add_dependencies(thplayer thtk)

get_property(thtk_bindir DIRECTORY thtk/ PROPERTY BINARY_DIR)
target_include_directories(thplayer PRIVATE ${thtk_bindir})

target_link_libraries(thplayer thtk Qt6::Widgets Qt6::Multimedia ICU::uc ICU::i18n)

if(WIN32)
    set_property(TARGET thplayer PROPERTY WIN32_EXECUTABLE True)
endif()
