#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QDebug>
#include <unistd.h>
#include "mainwindow.h"
#include <QMutex>
#include <alsa/asoundlib.h>

class MainWindow;

class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlayer(int channels, int frequency, int framebits = 16, QObject *parent = 0);
public slots:
    void play();

public:
    volatile bool mExitFlag;

    int mAudioChannels;
    int mAudioFrequency;
    int mAudioFramebits;
//    static snd_pcm_t* handle;
};

#endif // AUDIOPLAYER_H
