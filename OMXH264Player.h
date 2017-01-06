#ifndef OMXH264PLAYER_H
#define OMXH264PLAYER_H

#include <QObject>
#include "bcm_host.h"
extern "C"{
    #include "ilclient.h"
}

class OMXH264Player: public QObject
{
   Q_OBJECT
public:
    OMXH264Player(QObject *parent = 0);
    ~OMXH264Player();
    void draw( unsigned char * image, int size);
    void clear();
public slots:
    void playbackTest();

public:
    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
    COMPONENT_T *video_decode, *video_scheduler, *video_render, *clock;
    COMPONENT_T *list[5];
    TUNNEL_T tunnel[4];
    ILCLIENT_T *client;
    int port_settings_changed;
    int first_packet;
    OMX_BUFFERHEADERTYPE *omxbuf;
};

#endif // OMXH264PLAYER_H
