#-------------------------------------------------
#
# Project created by QtCreator 2016-06-04T13:08:06
#
#-------------------------------------------------

QT       += core gui network opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = middleware-qt
TEMPLATE = app

DEFINES += USE_OPENGL \
        USE_EGL \
        IS_RPI \
        STANDALONE \
        __STDC_CONSTANT_MACROS \
        __STDC_LIMIT_MACROS \
        TARGET_POSIX \
        _LINUX \
        _REENTRANT \
        _LARGEFILE64_SOURCE \
        _FILE_OFFSET_BITS=64 \
        HAVE_LIBOPENMAX=2 \
        OMX \
        OMX_SKIP64BIT \
        USE_EXTERNAL_OMX \
        HAVE_LIBBCM_HOST \
        USE_EXTERNAL_LIBBCM_HOST \
        USE_VCHIQ_ARM \
        AVCODEC_MAX_AUDIO_FRAME_SIZE=192000

INCLUDEPATH += /opt/vc/include \
        /opt/vc/include/interface/vcos/pthreads \
        /opt/vc/include/interface/vmcs_host/linux \
        /opt/vc/src/hello_pi/libs/ilclient \
        /opt/vc/src/hello_pi/libs/vgfont \


SOURCES += main.cpp\
    OMXH264Player.cpp \
    mainwindow.cpp \
    DataBuffer.cpp \
    videodec.cpp \
    jpegresize.cpp \
    AudioPlayer.cpp \
    AudioDec.cpp \
    GlPlayer.cpp \
    CommandHandler.cpp

HEADERS  += mainwindow.h \
    DataBuffer.h \
    videodec.h \
    jpegresize.h \
    audioplayer.h \
    audiodec.h \
    glplayer.h \
    OMXH264Player.h \
    CommandHandler.h

FORMS    += mainwindow.ui

LIBS += -L/opt/vc/src/hello_pi/libs/ilclient \
        -lilclient \
        -L/opt/vc/lib \
        -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt \
        -L/opt/vc/src/hello_pi/libs/vgfont -ldl \
        -lasound \
        -lavcodec \
        -lavdevice \
        -lavfilter \
        -lavformat \
        -lpostproc \
        -lswresample \
        -lswscale \
        -lavutil \
        -lvorbis \
        -lx264 -ltheoraenc -ltheoradec -lxvidcore -lvorbisfile -lvorbisenc \
        -lm -lz -lfaac -lmp3lame

CONFIG += console
