cmake_minimum_required(VERSION 3.14.0)
project(deduper CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(thtk EXCLUDE_FROM_ALL)

#they assumed their project is the root project...
target_include_directories(thtk PUBLIC thtk/)

find_package(Qt5 REQUIRED COMPONENTS Widgets Multimedia)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(thplayer
    main.cpp
    boundedbuffer.cpp
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
