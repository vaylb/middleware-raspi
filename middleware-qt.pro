#-------------------------------------------------
#
# Project created by QtCreator 2016-06-04T13:08:06
#
#-------------------------------------------------

QT       += core gui network opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = middleware-qt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    DataBuffer.cpp \
    videodec.cpp \
    jpegresize.cpp \
    AudioPlayer.cpp \
    AudioDec.cpp

HEADERS  += mainwindow.h \
    DataBuffer.h \
    videodec.h \
    jpegresize.h \
    audioplayer.h \
    audiodec.h

FORMS    += mainwindow.ui

LIBS += -lasound \
        -lavcodec \
        -lavdevice \
        -lavfilter \
        -lavformat \
        -lpostproc \
        -lswresample \
        -lswscale \
        -lavutil \
        -lvorbis \
        -lm -lz -lfaac -lmp3lame \
        -lx264 -ltheoraenc -ltheoradec -lxvidcore -lvorbisfile -lvorbisenc \

