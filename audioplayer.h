#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QDebug>
#include <unistd.h>
#include "mainwindow.h"
#include <QMutex>

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
};

#endif // AUDIOPLAYER_H
