#ifndef VIDEODEC_H
#define VIDEODEC_H
extern "C"{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/mem.h>
    #include <libavutil/fifo.h>
    #include <libswscale/swscale.h>
}
#include <QObject>
#include <QImage>
#include <QDebug>
#include <QQueue>
#include <unistd.h>
#include "mainwindow.h"
#include <QMutex>


class MainWindow;
class AudioPlayer;

class VideoDec : public QObject
{
    Q_OBJECT
public:
    explicit VideoDec(QObject *parent = 0);
    struct ImgPacket{
        QImage PImage;
        struct ImgPacket *next;
    };
    int videoindex;
    int audioindex;
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtxVideo;
    AVCodec *pCodecVideo;
    AVCodecContext *pCodecCtxAudio;
    AVCodec *pCodecAudio;
    AVPacket packet;
//    AVFrame *pFrame,*pFrameRGB;
    AVIOContext *avio;
    char *FileName;
    QQueue<QImage> videoImg;
    volatile bool exitFlag;
    QMutex mutex;

    QThread *mAudioPlayThread;
    AudioPlayer * mAudioPlayer;

signals:
    void SendImage(QImage img);
    void audio_play_s();

public slots:
    void init();
    void play();
};



#endif // VIDEODEC_H
