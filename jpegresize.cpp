#include "jpegresize.h"
#include <QApplication>
#include <QDesktopWidget>

JpegResize::JpegResize(QObject *parent) :
    QObject(parent),
    mExitFlag(false)
{
    QDesktopWidget *dwsktopwidget = QApplication::desktop();
    QRect deskrect = dwsktopwidget->availableGeometry();
    screenWidth = deskrect.width();
    screenHeight = deskrect.height();
}

void JpegResize::resize(){
    qDebug()<<"Resize run";
    QImage img;
    QImage pixmap;
    int width = 0,height = 0;
    while(!mExitFlag){
        if(framesIn.isEmpty()){ //if empty then sleep
            usleep(1000);
        }else{
            img = framesIn.dequeue();
            width = img.width();
            height = img.height();
//            qDebug()<<"Resize width = "<<width<<", height = "<<height;
            int scale_w = 0, scale_h = 0;
            if(height > width){
                scale_h = screenHeight-24;
                scale_w = (quint32)(width*scale_h/height);
            }else{
                scale_w = screenWidth-24;
                scale_h = (quint32)(height*scale_w/width);
            }
            pixmap = img.scaled(scale_w,scale_h,Qt::KeepAspectRatio,Qt::SmoothTransformation);
            if(!mExitFlag){
                mutex.lock();
                framesOut.append(pixmap);
                mutex.unlock();
                showFrame(scale_w, scale_h);
            }
        }
    }
    framesOut.clear();
    framesIn.clear();
}
