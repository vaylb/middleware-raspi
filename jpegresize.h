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

    static QQueue<QImage> framesIn;
//    QQueue<QPixmap> framesOut;
    static QQueue<QImage> framesOut;

    quint32 screenWidth;
    quint32 screenHeight;
    static bool mExitFlag;
    static QMutex mutex;

signals:
    void showFrame(int width, int height);
public slots:
    void resize();

};

#endif // JPEGRESIZE_H
