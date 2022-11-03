cmake_minimum_required(VERSION 3.14.0)
project(thplayer CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)
option(WITH_OPENMP OFF)
add_subdirectory(thtk EXCLUDE_FROM_ALL)

#they assumed their project is the root project...
target_include_directories(thtk PUBLIC thtk/)

find_package(Qt5 REQUIRED COMPONENTS Widgets Multimedia)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

INCLUDE (CheckTypeSize)

SET(CMAKE_EXTRA_INCLUDE_FILES "filesystem")
check_type_size("std::filesystem::path::value_type" PATH_VALSIZE LANGUAGE CXX)
SET(CMAKE_EXTRA_INCLUDE_FILES)

add_compile_definitions(PATH_VALSIZE=${PATH_VALSIZE})

add_executable(thplayer
    main.cpp
    loopedpcmstreamer.cpp
    mainwindow.cpp
    outputselectiondialog.cpp
    songlist.cpp
    thdatwrapper.cpp

    mainwindow.ui
    outputselectiondialog.ui

    res.qrc
)

add_dependencies(thplayer thtk)

get_property(thtk_bindir DIRECTORY thtk/ PROPERTY BINARY_DIR)
target_include_directories(thplayer PRIVATE ${thtk_bindir})

target_link_libraries(thplayer thtk Qt5::Widgets Qt5::Multimedia)

if(WIN32)
    set_property(TARGET thplayer PROPERTY WIN32_EXECUTABLE True)
endif()
