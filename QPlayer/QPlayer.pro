#-------------------------------------------------
#
# Project created by QtCreator 2017-07-06T10:36:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QPlayer
TEMPLATE = app


SOURCES += main.cpp \
    mainwindow.cpp \
    qplayer.cpp \
    playwidget.cpp

HEADERS  += mainwindow.h \
    qplayer.h \
    playwidget.h

FORMS    += mainwindow.ui \
    playwidget.ui

INCLUDEPATH += C:/ffmpeg/include
LIBS += C:/ffmpeg/bin/avformat.lib      \
        C:/ffmpeg/bin/avutil.lib        \
        C:/ffmpeg/bin/swscale.lib       \
        C:/ffmpeg/bin/swresample.lib    \
        C:/ffmpeg/bin/avcodec.lib
