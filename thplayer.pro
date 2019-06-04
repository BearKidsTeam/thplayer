#-------------------------------------------------
#
# Project created by QtCreator 2017-10-03T16:21:39
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = thplayer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    boundedbuffer.cpp \
    thtk/thtk/bits.c \
    thtk/thtk/error.c \
    thtk/thtk/io.c \
    thtk/thtk/rng_mt.c \
    thtk/thtk/thcrypt.c \
    thtk/thtk/thcrypt105.c \
    thtk/thtk/thdat.c \
    thtk/thtk/thdat02.c \
    thtk/thtk/thdat06.c \
    thtk/thtk/thdat08.c \
    thtk/thtk/thdat95.c \
    thtk/thtk/thdat105.c \
    thtk/thtk/thlzss.c \
    thtk/thtk/thrle.c \
    thtk/thtk/util.c \
    thdatwrapper.cpp \
    songlist.cpp \
    outputselectiondialog.cpp

HEADERS += \
    boundedbuffer.hpp \
    mainwindow.hpp \
    thtk/thtk/bits.h \
    thtk/thtk/dat.h \
    thtk/thtk/error.h \
    thtk/thtk/io.h \
    thtk/thtk/rng_mt.h \
    thtk/thtk/thcrypt.h \
    thtk/thtk/thcrypt105.h \
    thtk/thtk/thdat.h \
    thtk/thtk/thlzss.h \
    thtk/thtk/thrle.h \
    thtk/thtk/thtk.h \
    thtk/thtk/util.h \
    thdatwrapper.hpp \
    songlist.hpp \
    outputselectiondialog.hpp

FORMS += \
        mainwindow.ui \
    outputselectiondialog.ui

unix {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
    BINDIR = $$PREFIX/bin
    target.path = $$BINDIR
    INSTALLS += target
    QMAKE_CXXFLAGS += -std=c++11 -Wall
}

macx {
    ICON = assets/thplayer.icns
}

RESOURCES += \
    res.qrc
INCLUDEPATH += \
    thtk \
    thtk-config

RC_ICONS = assets/thplayer.ico
