#include "jpegresize.h"
#include <QApplication>
#include <QDesktopWidget>

JpegResize::JpegResize(QObject *parent) :
    QObject(parent)
{
    QDesktopWidget *dwsktopwidget = QApplication::desktop();
    QRect deskrect = dwsktopwidget->availableGeometry();
    screenWidth = deskrect.width();
    screenHeight = deskrect.height();
}

void JpegResize::resize(){
    //qDebug() << "start resize";
    QImage img;
    QImage imgtemp;
    QImage pixmap;
    int width = 0,height = 0;
    int contentWidth = 0, contentHeight = 0;
    while(true){
        if(framesIn.isEmpty()){ //if empty then sleep
            usleep(200000);
        }else{
            img = framesIn.dequeue();
            width = img.width();
            height = img.height();
            //qDebug() << "file receive success, width = "<<width<<" height = "<<height<<"\n";
            if((width == 1080 && height == 720) || (width == 1920 && height == 1080)){
//                videoShow->setFixedWidth(deskrect.width()*3/4);
//                videoShow->setFixedHeight(deskrect.height()*3/4);
                imgtemp =  img.copy(0,height/2-20,width-height/2,height/2+20);
//                imgtemp =  img->copy(0,height/2+100,width-height/2-100,height/2-40);
                contentWidth = screenWidth*3/4;
                contentHeight = screenHeight*3/4;
            }else{
//                videoShow->setFixedWidth(width);
//                videoShow->setFixedHeight(deskrect.height());
                //QTime time2;
                //time2.start();
                imgtemp = img.copy(55,0,width-110,height);
                contentWidth = width;
                contentHeight = screenHeight;

                //qDebug() << "copy  and scale img cost time:"<<time2.elapsed()<<"ms";
            }
            pixmap = imgtemp.scaled(width,screenHeight,Qt::KeepAspectRatio,Qt::SmoothTransformation);

            framesOut.append(pixmap);
            showFrame(contentWidth, contentHeight);
        }
    }
}
