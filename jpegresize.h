#ifndef JPEGRESIZE_H
#define JPEGRESIZE_H

#include <QObject>
#include <QImage>
#include <QDebug>
#include <QQueue>
#include <QMutex>
#include <unistd.h>
#include "mainwindow.h"

class MainWindow;

class JpegResize : public QObject
{
    Q_OBJECT
public:
    explicit JpegResize(QObject *parent = 0);

    QQueue<QImage> framesIn;
//    QQueue<QPixmap> framesOut;
    QQueue<QImage> framesOut;

    quint32 screenWidth;
    quint32 screenHeight;
    bool mExitFlag;
    bool mTimeFlag;
    QMutex mutex;

signals:
    void showFrame(int width, int height);
public slots:
    void resize();

};

#endif // JPEGRESIZE_H
