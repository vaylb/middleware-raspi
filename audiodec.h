#ifndef AUDIODEC_H
#define AUDIODEC_H

#include <QObject>
#include <QDebug>
#include <unistd.h>
#include "mainwindow.h"
#include <QMutex>

class MainWindow;

class AudioDec : public QObject
{
    Q_OBJECT
public:
    explicit AudioDec(QObject *parent = 0);
public slots:
    void decode();

public:
    volatile bool mExitFlag;
};

#endif // AUDIODEC_H
