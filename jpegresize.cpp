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
    //qDebug() << "start resize";
    QImage img;
//    QImage imgtemp;
    QImage pixmap;
    int width = 0,height = 0;
    while(!mExitFlag){
        if(framesIn.isEmpty()){ //if empty then sleep
            usleep(200000);
        }else{
            img = framesIn.dequeue();
            width = img.width();
            height = img.height();
//            qDebug()<<"Resize width = "<<width<<", height = "<<height;
//            imgtemp = img.copy(0,0,width,height);
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
                framesOut.append(pixmap);
                showFrame(scale_w, scale_h);
            }
        }
    }
    framesOut.clear();
    framesIn.clear();
}
