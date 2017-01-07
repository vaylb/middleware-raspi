#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QImage>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>

//for screen metric
#include <QApplication>
#include <QDesktopWidget>
#include <QTime>
//#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

DataBuffer* MainWindow::video_compressed_data_pool = new DataBuffer(32768*32*8);//32k*16=1024*8k
DataBuffer* MainWindow::mDataPool = new DataBuffer(1024*4*16);
DataBuffer* MainWindow::mPcmPool = new DataBuffer(AVCODEC_MAX_AUDIO_FRAME_SIZE);

MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent),
    window_width(0),
    window_height(0),
    mVideoStartPlayFlag(false),
    bytesReceived(0),
    setupbytesReceived(0),
    mJpegResize(NULL),
    mShowJpegFlag(true),
    mSetupFlag(false),
    mAudioStartFlag(false),
    mAudioPlayBackFlag(false),
    videoDecThread(NULL),
    audioPlayThread(NULL),
    audioDecodeThread(NULL),
    mVideoDec(NULL),
    readTimer(NULL),
    filetotalbytes(0),
    filebytesreceived(0),
    mAudioDataFileFlag(false),
    mAudioDataFileTotalBytes(0),
    mAudioDataFileReceived(0),
    mAudioPlayer(NULL),
    mAudioDec(NULL),
    mVideoDataFileReceived(0),
    mVideoDataFileTotalBytes(0)
{
//    setenv("EGL_FORCE_DRI3","1",1);
    videoShow = new QLabel;
    videoShowTips = new QLabel;
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(videoShow, 0, Qt::AlignCenter|Qt::AlignBottom);
    videoShow->setText("中间件系统");
    videoShow->setStyleSheet("font-size:40px");
    mainLayout->addWidget(videoShowTips,0, Qt::AlignCenter|Qt::AlignTop);
    videoShowTips->setText("等待连接...");
    videoShowTips->setStyleSheet("font-size:20px");
    setLayout(mainLayout);
    setWindowTitle("中间件系统");
    setWindowFlags(Qt::FramelessWindowHint);//去掉标题栏*/
    QDesktopWidget *dwsktopwidget = QApplication::desktop();
    QRect deskrect = dwsktopwidget->availableGeometry();
    window_width = deskrect.width();
    window_height = deskrect.height();
    setFixedSize(window_width,window_height); //设置窗体固定大小
//    gl = new CPlayWidget(this);
//    gl->PlayOneFrame();
//    mainLayout->addWidget(gl, 0, Qt::AlignCenter|Qt::AlignBottom);
    printscreeninfo();

    startListening();

    //test omx player
//    QThread *H264PlayThread = new QThread(this);
//    OMXH264Player* H264Player = new OMXH264Player();
//    H264Player->moveToThread(H264PlayThread);

//    connect(this,SIGNAL(h264_play_s()),H264Player,SLOT(playbackTest()));
//    H264PlayThread->start();
//    emit h264_play_s();
}

MainWindow::~MainWindow()
{
}

void MainWindow::startListening(){
    qDebug()<<"udp bind start";
    device_scan_receiver = new QUdpSocket(this);
    device_scan_port = 40001;
    bool result = device_scan_receiver->bind(device_scan_port);
    if(!result) {
        qDebug()<<"udp bind error\n";
        return ;
    }
    connect(device_scan_receiver, SIGNAL(readyRead()),this, SLOT(device_scan_come()));

    //jpeg数据
    video_data_port = 40004;
    video_data_receiver = new QTcpSocket(this);
    connect(video_data_receiver, SIGNAL(readyRead()), this, SLOT(receive_video_data()));
    connect(video_data_receiver, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));
    //h264数据
    video_compressed_data_port = 40005;
    video_compressed_data_receiver = new QTcpSocket(this);
    connect(video_compressed_data_receiver, SIGNAL(readyRead()), this, SLOT(receive_compressed_video_data()));
    connect(video_compressed_data_receiver, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));

    //setup
    device_setup_port = 39999;
    device_setup_receiver = new QTcpSocket(this);
    connect(device_setup_receiver, SIGNAL(readyRead()), this, SLOT(receive_setup_data()));
    connect(device_setup_receiver, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));

    //audio
    audio_data_port =40003;
    audio_data_receiver = new QTcpSocket(this);
    connect(audio_data_receiver, SIGNAL(readyRead()), this, SLOT(receive_audio_data()));
    connect(audio_data_receiver, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));

    //file print
    file_data_port = 40006;
    file_data_receiver = new QTcpSocket(this);
    connect(file_data_receiver, SIGNAL(readyRead()), this, SLOT(receive_file_data()));
    connect(file_data_receiver, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));
}

void MainWindow::on_pushButton_clicked()
{
    //显示图片
    QImage *image=new QImage("/home/vaylb/图片/BingWallpaper-2015-03-10.jpg");

    QImage* imgScaled = new QImage;
    *imgScaled = image->scaled(640,480,Qt::KeepAspectRatio);

    QLabel *label=new QLabel(this);
    label->setGeometry(0,0,640,480);
    label->setPixmap(QPixmap::fromImage(*imgScaled));
    label->show();
}

void MainWindow::slotClose()
{
    device_scan_receiver->close();
}

void MainWindow::device_scan_come()
{
    while(device_scan_receiver->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress *hostaddr = new QHostAddress;
        quint16 * hostport = new  quint16;
        datagram.resize(device_scan_receiver->pendingDatagramSize());

        device_scan_receiver->readDatagram(datagram.data(), datagram.size(),hostaddr,hostport);
        QString host_ip = hostaddr->toString();
        host_ip = host_ip.mid(host_ip.lastIndexOf(":")+1);
        mHostAddr = host_ip;
        qDebug()<<"device scan, receive:"<<datagram.data()<<", ip:"<<host_ip;
        if(strcmp(datagram.data(),"a")==0){
            //response to device scan
            int type = getDeviceType();
            char mac[18];
            get_mac(mac);
            mac[17] = '\0';

            QByteArray msg="{\"name\":\"01\",\"type\":2}";
            msg.replace(20,1,QString::number(type).toStdString().c_str());
            msg.replace(9,2,mac);

            QUdpSocket *device_scan_join_up = new QUdpSocket(this);
            qDebug()<<"res msg:"<<msg;
            device_scan_join_up->writeDatagram(msg.data(), msg.size(),*hostaddr, 40002);
            delete device_scan_join_up;
            videoShowTips->setText("已连接至终端设备");
        }
        else if(strcmp(datagram.data(),"c")==0){
            videoShow->setText("");
            videoShowTips->setHidden(true);

            qDebug()<<"init jpegResizeThread";
            jpegResizeThread = new QThread(this);
            mJpegResize = new JpegResize();
            mJpegResize->moveToThread(jpegResizeThread);
            connect(this,SIGNAL(resize_s()),mJpegResize,SLOT(resize()));
            connect(mJpegResize,SIGNAL(showFrame(int,int)),this,SLOT(showJpeg(int,int)));
            jpegResizeThread->start();
            emit resize_s();
            //connect to mobile phone with tcp
            video_data_receiver->connectToHost(*hostaddr, video_data_port);//连接到指定ip地址和端口的主机
        }
        else if(strcmp(datagram.data(),"d")==0){
//            this->show();
            videoShow->setText("");
            mainLayout->setAlignment(videoShow,Qt::AlignCenter);
            videoShowTips->setHidden(true);
            mVideoDec = new VideoDec();
            video_compressed_data_receiver->connectToHost(*hostaddr, video_compressed_data_port);
        }
        else if(!mSetupFlag && strcmp(datagram.data(),"g")==0){
            videoShowTips->setText("正在设置网络连接...");
            mSetupFlag = true;
            //connect to mobile phone with tcp
            device_setup_receiver->connectToHost(*hostaddr, device_setup_port);
        }
        else if(!mAudioPlayBackFlag && strcmp(datagram.data(),"b")==0){
            videoShowTips->setText("播放音乐...");
            mAudioPlayBackFlag  = true;
            mAudioDataFileFlag = true;
            mKAudioType = TYPE_AAC;
            mAudioPlayer = new AudioPlayer(2, 44100);
            mAudioDec = new AudioDec();
            audio_data_receiver->connectToHost(*hostaddr, audio_data_port);
        }
        else if(!mAudioPlayBackFlag && strcmp(datagram.data(),"e")==0){
            videoShowTips->setText("播放音乐...");
            mAudioPlayBackFlag  = true;
            mKAudioType = TYPE_PCM;
            mPcmPool = mDataPool;
            mAudioPlayer = new AudioPlayer(1, 48000);
            audio_data_receiver->connectToHost(*hostaddr, audio_data_port);
        }
        else if(strcmp(datagram.data(),"f")==0){          
            QDesktopWidget *dwsktopwidget = QApplication::desktop();
            QRect deskrect = dwsktopwidget->availableGeometry();
            videoShow->setFixedWidth(deskrect.width());
            videoShow->setFixedHeight(deskrect.height()/2);
            videoShow->setText("中间件系统");
            videoShow->setAlignment(Qt::AlignCenter|Qt::AlignBottom);
            videoShowTips->setHidden(false);
            videoShowTips->setText("播放结束");
            videoShowTips->setAlignment(Qt::AlignCenter|Qt::AlignTop);


            mShowJpegFlag = false;
            if(mVideoDec != NULL){
                sendMessage(getJobDoneMsg());
                video_compressed_data_receiver->close();
                mVideoStartPlayFlag = false;
                mVideoDataFileReceived = 0;
                mVideoDataFileTotalBytes = 0;
                if(readTimer != NULL){
                    readTimer->stop();
                }
                mVideoDec->videoImg.clear();
                mVideoDec->exitFlag = true;
                if(videoDecThread != NULL){
                    videoDecThread->exit();
                    videoDecThread = NULL;
                }
                mVideoDec = NULL;
                video_compressed_data_pool->Reset();
            }

            if(mAudioPlayer != NULL){
                mAudioPlayer->mExitFlag = true;
                audioPlayThread->exit();
                mAudioPlayer = NULL;
                audioPlayThread = NULL;
                if(mAudioDec != NULL){
                    mAudioDec->mExitFlag = true;
                    audioDecodeThread->exit();
                    mAudioDec = NULL;
                    audioDecodeThread = NULL;
                }
                mAudioDataFileFlag = false;
                mAudioDataFileReceived = 0;
                mAudioDataFileTotalBytes = 0;
                mAudioStartFlag = false;
                mAudioPlayBackFlag = false;
                audio_data_receiver->close();
                sendMessage(getJobDoneMsg());
                mDataPool->Reset();
                mPcmPool->Reset();
            }
        }
        else if(strcmp(datagram.data(),"h")==0){
            videoShowTips->setText("文件打印...");
            mPrintFileType = TYPE_FILE;
            file_data_receiver->connectToHost(*hostaddr, file_data_port);
        }
        else if(strcmp(datagram.data(),"i")==0){
            videoShowTips->setText("安装打印驱动...");
            mPrintFileType = TYPE_DRIVER;
            file_data_receiver->connectToHost(*hostaddr, file_data_port);
        }
        delete hostaddr;
        delete hostport;
    }
}

void MainWindow::receive_video_data()
{
    if(bytesReceived < sizeof(qint32)){
        instream = new QDataStream(video_data_receiver);
        if((video_data_receiver->bytesAvailable() >= sizeof(qint32))){
            *instream >> TotalBytes;
            bytesReceived += sizeof(qint32);
//            qDebug() <<"receive new image size:"<<TotalBytes;
        }
        imagedata.resize(0);
        imagebuffer = new QBuffer(&imagedata);
        imagebuffer->open(QIODevice::WriteOnly);
        return;
    }

    if (bytesReceived < TotalBytes){
        qint32  available = video_data_receiver->bytesAvailable();
//        qDebug() <<"image available:"<<available;
        if((bytesReceived+available)>TotalBytes){
            available = TotalBytes-bytesReceived;
            inBlock = video_data_receiver->read(available);
        }else{
            inBlock = video_data_receiver->readAll();
        }

        bytesReceived += available;
        imagebuffer->write(inBlock,available);
        inBlock.resize(0);
    }

    if (bytesReceived == TotalBytes) {
        imagebuffer->close();
        imagebuffer->open(QIODevice::ReadOnly);

        QImage img;
        int ret = img.loadFromData(imagebuffer->data());
        if(ret){
            mJpegResize->framesIn.append(img);
        }
        imagebuffer->close();
        delete imagebuffer;
        bytesReceived = 0;
        TotalBytes = 0;
    }
}

void MainWindow::showJpeg(int width, int height){
    if(mShowJpegFlag && !mJpegResize->framesOut.isEmpty()){
        //qDebug()<<"showJpeg width = "<<width<<", height = "<<height;
        videoShow->setFixedWidth(width);
        videoShow->setFixedHeight(height);
        videoShow->setPixmap(QPixmap::fromImage(mJpegResize->framesOut.dequeue()));
    }
}

// 错误处理
//QAbstractSocket类提供了所有scoket的通用功能，socketError为枚举型
void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    qDebug() <<"error msg:"<< video_compressed_data_receiver->errorString();
}

//receive compressed video data and start thread to decodec
void MainWindow::receive_compressed_video_data()
{
    if(mVideoDataFileReceived < sizeof(quint32)){
        QDataStream *instream = new QDataStream(video_compressed_data_receiver);
        if((video_compressed_data_receiver->bytesAvailable() >= sizeof(quint32))){
            *instream >> mVideoDataFileTotalBytes;
            mVideoDataFileReceived += sizeof(quint32);
            qDebug() <<"receive new video file size:"<<mVideoDataFileTotalBytes;
        }
        return;
    }
    while(mVideoDec != NULL && !mVideoDec->exitFlag && video_compressed_data_receiver->bytesAvailable() > 0){
        qint32 available = video_compressed_data_receiver->bytesAvailable();
        qint32 data_pool_can_write = 0;
        while((data_pool_can_write = video_compressed_data_pool->getWriteSpace()) <=0){
            usleep(100000);
        }

        qint32 read_temp = available>data_pool_can_write?data_pool_can_write:available;

        if(read_temp > 0){
            if(mVideoDataFileReceived + read_temp > mVideoDataFileTotalBytes){
                read_temp = mVideoDataFileTotalBytes - mVideoDataFileReceived;
            }
            mVideoDataFileReceived += read_temp;
            inBlock = video_compressed_data_receiver->read(read_temp);
            video_compressed_data_pool->Write(inBlock.data(),read_temp);
            inBlock.resize(0);
        }

        if(!mVideoStartPlayFlag && video_compressed_data_pool->getReadSpace()>1024*128){
            mVideoStartPlayFlag = true;
            if(videoDecThread == NULL){
                videoDecThread = new QThread(this);
            }
            if(readTimer == NULL){
                readTimer = new QTimer(this);
            }
            mVideoDec->moveToThread(videoDecThread);

            connect(this,SIGNAL(init_s()),mVideoDec,SLOT(init()));
            connect(readTimer,SIGNAL(timeout()),this,SLOT(showVideo()));
            qDebug()<<"emit start signal";
            videoDecThread->start();
            emit init_s();
            readTimer->start(35);
        }

        if(mVideoDataFileReceived == mVideoDataFileTotalBytes){ //&& video_compressed_data_pool->getReadSpace() == 0
            qDebug() <<"video playback complete, media size:"+QString::number(mVideoDataFileReceived);
            video_compressed_data_receiver->readAll();
            video_compressed_data_receiver->close();

        }else{
//            qDebug() <<"video playback emit ready signal, current size:"+QString::number(mVideoDataFileReceived);
            usleep(10000);
            emit video_compressed_data_receiver->readyRead();
        }
    }
}

void MainWindow::showVideo()
{
    if(mVideoDec == NULL){
        return;
    }

    if(mVideoDataFileTotalBytes >0 && mVideoDataFileReceived == mVideoDataFileTotalBytes && video_compressed_data_pool->getReadSpace() == 0){
        logDebug("video file playback complete");
        mVideoStartPlayFlag = false;
        mVideoDataFileReceived = 0;
        mVideoDataFileTotalBytes = 0;
        readTimer->stop();
        mVideoDec->videoImg.clear();
        mVideoDec->exitFlag = true;
        videoDecThread->exit();

        mVideoDec = NULL;
        videoDecThread = NULL;
        video_compressed_data_pool->Reset();
        sendMessage(getJobDoneMsg());
        videoShow->setFixedWidth(window_width);
        videoShow->setFixedHeight(window_height/2);
        videoShow->setText("中间件系统");
        videoShow->setAlignment(Qt::AlignCenter|Qt::AlignBottom);
        videoShowTips->setHidden(false);
        videoShowTips->setText("播放结束");
        videoShowTips->setAlignment(Qt::AlignCenter|Qt::AlignTop);
        return;
    }

    if(mVideoDec->exitFlag || mVideoDec->videoImg.isEmpty()) {
        if(mVideoDec->exitFlag){
            qDebug()<<"video show stop";
            readTimer->stop();
            mVideoDec->videoImg.clear();
        }
        return;
    }

    QPixmap pix;
    mVideoDec->mutex.lock();
    pix = pix.fromImage(mVideoDec->videoImg.dequeue());
    mVideoDec->mutex.unlock();
//    pix = pix.scaled(QSize(window_width,window_height),Qt::KeepAspectRatio, Qt::SmoothTransformation);
    videoShow->setFixedSize(window_width,window_height);
    videoShow->setScaledContents(true);
    videoShow->setPixmap(pix);
}

void MainWindow::receive_setup_data(){
    if(setupbytesReceived < sizeof(qint32)){
        setupinstream = new QDataStream(device_setup_receiver);
        if((device_setup_receiver->bytesAvailable() >= sizeof(qint32))){
            *setupinstream >> setupTotalBytes;
            qDebug()<<"setup msg size = "<<setupTotalBytes;
            setupbytesReceived += sizeof(qint32);
        }
        setupdata.resize(0);
        setupbuffer = new QBuffer(&setupdata);
        setupbuffer->open(QIODevice::WriteOnly);
        return;
    }

    if (setupbytesReceived < setupTotalBytes){
        qint32  available = device_setup_receiver->bytesAvailable();
        if((setupbytesReceived+available)>setupTotalBytes){
            available = setupTotalBytes-setupbytesReceived;
            setupinBlock = device_setup_receiver->read(available);
        }else{
            setupinBlock = device_setup_receiver->readAll();
        }

        setupbytesReceived += available;
        setupbuffer->write(setupinBlock,available);
        setupinBlock.resize(0);
    }

    if (setupbytesReceived == setupTotalBytes) {
        setupbuffer->close();
        setupbuffer->open(QIODevice::ReadOnly);
        QString wifi =setupbuffer->data();
        QString name = wifi.section(':',0,0);
        QString psw = wifi.section(':',1,1);
        qDebug()<<"name:"<<name.toStdString().c_str()<<"psw:"<<psw.toStdString().c_str();
        QString cmd1("sudo sed -i '6c ssid=\"");
        cmd1.append(name);
        cmd1.append("\"' /etc/wpa_supplicant/wpa_supplicant.conf");
        QString cmd2("sudo sed -i '7c psk=\"");
        cmd2.append(psw);
        cmd2.append("\"' /etc/wpa_supplicant/wpa_supplicant.conf");
        qDebug()<<cmd1.toStdString().c_str();
        qDebug()<<cmd2.toStdString().c_str();
        system(cmd1.toStdString().c_str());
        system(cmd2.toStdString().c_str());
        system("sudo wpa_supplicant -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf");
        setupbuffer->close();
        delete setupbuffer;
        setupbytesReceived = 0;
        setupTotalBytes = 0;
    }
}

void MainWindow::receive_audio_data(){
//    if(!mAudioDataFileFlag){
//        qint32 available = audio_data_receiver->bytesAvailable();
//        qint32 data_pool_can_write  = 0;
//        while((data_pool_can_write = mDataPool->getWriteSpace()) <=0){
//            usleep(2000);
//        }

//        if(data_pool_can_write==0) {
//    //        qDebug()<<"new data but datapool can not write, can read "<<mDataPool->getReadSpace();
//            return;
//        }

//        qint32 read_temp = available>data_pool_can_write?data_pool_can_write:available;
//        if(read_temp>4096){
//            read_temp = 4096;
//        }
//        audioInBlock = audio_data_receiver->read(read_temp);
//        mDataPool->Write(audioInBlock.data(),read_temp);
//        audioInBlock.resize(0);
//        //qDebug() <<"data available:"<<available<<", datapool has:"<<video_compressed_data_pool->getReadSpace();

//        if(!mAudioStartFlag && mDataPool->getReadSpace()>4096*8){
//            mAudioStartFlag = true;
//            audioPlayThread = new QThread(this);
//            mAudioPlayer->moveToThread(audioPlayThread);

//            connect(this,SIGNAL(audio_play_s()),mAudioPlayer,SLOT(play()));
//            audioPlayThread->start();
//            emit audio_play_s();

//            if(mKAudioType == TYPE_AAC){
//                audioDecodeThread = new QThread(this);
//                mAudioDec->moveToThread(audioDecodeThread);
//                connect(this,SIGNAL(decode_s()),mAudioDec,SLOT(decode()));
//                audioDecodeThread->start();
//                emit decode_s();
//            }
//        }
//    }else
    if(mAudioDataFileFlag){
        if(mAudioDataFileReceived < sizeof(quint32)){
            QDataStream *instream = new QDataStream(audio_data_receiver);
            if((audio_data_receiver->bytesAvailable() >= sizeof(quint32))){
                *instream >> mAudioDataFileTotalBytes;
                mAudioDataFileReceived += sizeof(quint32);
                qDebug() <<"receive new audio file size:"<<mAudioDataFileTotalBytes;
            }
            return;
        }
    }
    while(mAudioPlayer != NULL && !mAudioPlayer->mExitFlag && audio_data_receiver->bytesAvailable()>0){
        qint32 available = audio_data_receiver->bytesAvailable();
        qint32 data_pool_can_write  = 0;
        while((data_pool_can_write = mDataPool->getWriteSpace()) <=0){
            usleep(100000);
        }

        qint32 read_temp = available>data_pool_can_write?data_pool_can_write:available;
        if(read_temp > 0){
            if(mAudioDataFileFlag){
                if(mAudioDataFileReceived+read_temp > mAudioDataFileTotalBytes){
                    read_temp = mAudioDataFileTotalBytes-mAudioDataFileReceived;
                }
                mAudioDataFileReceived += read_temp;
            }
            //logDebug("receive data:"+QString::number(read_temp));
            audioInBlock = audio_data_receiver->read(read_temp);
            mDataPool->Write(audioInBlock.data(),read_temp);
            audioInBlock.resize(0);
        }

        if(!mAudioStartFlag && mDataPool->getReadSpace()>4096*8){
            mAudioStartFlag = true;
            audioPlayThread = new QThread(this);
            mAudioPlayer->moveToThread(audioPlayThread);
            connect(this,SIGNAL(audio_play_s()),mAudioPlayer,SLOT(play()));
            audioPlayThread->start();
            emit audio_play_s();

            if(mKAudioType == TYPE_AAC){
                audioDecodeThread = new QThread(this);
                mAudioDec->moveToThread(audioDecodeThread);
                connect(this,SIGNAL(decode_s()),mAudioDec,SLOT(decode()));
                audioDecodeThread->start();
                emit decode_s();
            }
        }

        if(mAudioDataFileFlag){
            if(mAudioDataFileReceived == mAudioDataFileTotalBytes && mPcmPool->getReadSpace() == 0){
                qDebug() <<"audio playback complete";
                mAudioDataFileReceived = 0;
                mAudioDataFileTotalBytes = 0;
                audio_data_receiver->readAll();
                audio_data_receiver->close();
                sendMessage(getJobDoneMsg());

                mAudioStartFlag = false;
                mAudioPlayBackFlag = false;
                mAudioDataFileFlag = false;
                mAudioPlayer->mExitFlag = true;
                mAudioDec->mExitFlag = true;
                audioPlayThread->exit();
                audioDecodeThread->exit();

                mAudioPlayer = NULL;
                mAudioDec = NULL;
                audioPlayThread = NULL;
                audioDecodeThread = NULL;
                mDataPool->Reset();
                mPcmPool->Reset();
            }else{
                usleep(10000);
                emit audio_data_receiver->readyRead();
            }
        }
        usleep(2000);
    }
}

void MainWindow::receive_file_data()
{
    if(filebytesreceived < sizeof(quint32)){
        fileinstream = new QDataStream(file_data_receiver);
        if((file_data_receiver->bytesAvailable() >= sizeof(quint32))){
            *fileinstream >> filetotalbytes;
            filebytesreceived += sizeof(quint32);
            qDebug() <<"receive new file size:"<<filetotalbytes;
        }
        filedata.resize(0);
        filebuffer = new QBuffer(&filedata);
        filebuffer->open(QIODevice::WriteOnly);
        return;
    }

    if (filebytesreceived < filetotalbytes){
        qint32  available = file_data_receiver->bytesAvailable();
//        qDebug() <<"image available:"<<available;
        if((filebytesreceived+available)>filetotalbytes){
            available = filetotalbytes-filebytesreceived;
            fileinblock = file_data_receiver->read(available);
        }else{
            fileinblock = file_data_receiver->readAll();
        }

        filebytesreceived += available;
        filebuffer->write(fileinblock,available);
        fileinblock.resize(0);
    }

    if (filebytesreceived == filetotalbytes) {
        filebuffer->close();
        filebuffer->open(QIODevice::ReadOnly);

        QDateTime time = QDateTime::currentDateTime();
        QString filename;
        QString filefixname;
        if(mPrintFileType == TYPE_FILE){
            filename = "/home/pi/middleware/fileprint/"+time.toString("yyyy-MM-dd-hh-mm-ss")+".pdf";
            filefixname = "/home/pi/middleware/fileprint/"+time.toString("yyyy-MM-dd-hh-mm-ss")+"_fix.pdf";
        }else if(mPrintFileType == TYPE_DRIVER){
            filename = "/home/pi/middleware/drivers/"+time.toString("yyyy-MM-dd-hh-mm-ss")+".deb";
        }

        QFile *file = new QFile(filename);
        if(!file->open(QIODevice::WriteOnly)){
            qDebug() <<"open file fail";
        }
        file->write(filebuffer->data());
        file->close();
        qDebug() <<"file receive success";

        if(mPrintFileType == TYPE_FILE){
            QString command_fix = "sudo pdftk "+filename+" output "+filefixname;
            system(command_fix.toStdString().c_str());
            QString command_rm = "sudo rm "+filename;
            system(command_rm.toStdString().c_str());
            QString command_print = "sudo lp -d HP_1010 "+filefixname;
            system(command_print.toStdString().c_str());
        } else if(mPrintFileType == TYPE_DRIVER){
            QString command_driver = "sudo dpkg -i "+filename;
            system(command_driver.toStdString().c_str());
        }

        filebuffer->close();
        delete filebuffer;
        filebytesreceived = 0;
        filetotalbytes = 0;
        file_data_receiver->close();

        sendMessage(getJobDoneMsg());
    }
}

void MainWindow::sendMessage(QString msg){
    if(mHostAddr != NULL){
        QUdpSocket *socket = new QUdpSocket(this);
        QByteArray send = msg.toLatin1();
        qDebug()<<"response message:"<<send;
        QHostAddress addr(mHostAddr);
        socket->writeDatagram(send.data(), send.size(),addr, 40002);
        delete socket;
    }
}

QString MainWindow::getJobDoneMsg(){
    QString msg = "{\"type\":"+QString::number(getDeviceType())+",\"stat\":0}"; //stat 0-idle,1-playback
    return msg;
}

void MainWindow::printscreeninfo(){
    QDesktopWidget *dwsktopwidget = QApplication::desktop();
    QRect deskrect = dwsktopwidget->availableGeometry();
    QRect screenrect = dwsktopwidget->screenGeometry();
    int scrcount = dwsktopwidget->screenCount();
    qCritical("screenrect.w==%s",qPrintable(QString::number(screenrect.width())));
    qCritical("screenrect.h==%s",qPrintable(QString::number(screenrect.height())));
    qCritical("deskrect.w==%s",qPrintable(QString::number(deskrect.width())));
    qCritical("deskrect.h==%s",qPrintable(QString::number(deskrect.height())));
    qCritical("scrcount==%s",qPrintable(QString::number(scrcount)));
}

//获取地址
//返回MAC地址字符串
//返回：0=成功，-1=失败
int MainWindow::get_mac(char* mac){
    struct ifreq tmp;
    int sock_mac;
    char mac_addr[30];
    sock_mac = socket(AF_INET, SOCK_STREAM, 0);
    if( sock_mac == -1)
    {
        perror("create socket fail\n");
        return -1;
    }
    memset(&tmp,0,sizeof(tmp));
    strncpy(tmp.ifr_name,"eth0",sizeof(tmp.ifr_name)-1 );
    if( (ioctl( sock_mac, SIOCGIFHWADDR, &tmp)) < 0 )
    {
        printf("mac ioctl error\n");
        return -1;
    }
    sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)tmp.ifr_hwaddr.sa_data[0],
            (unsigned char)tmp.ifr_hwaddr.sa_data[1],
            (unsigned char)tmp.ifr_hwaddr.sa_data[2],
            (unsigned char)tmp.ifr_hwaddr.sa_data[3],
            (unsigned char)tmp.ifr_hwaddr.sa_data[4],
            (unsigned char)tmp.ifr_hwaddr.sa_data[5]
            );
    ::close(sock_mac);
    memcpy(mac,mac_addr,strlen(mac_addr));
    return 0;
}

int MainWindow::getDeviceType(){
    int res = 1;
    QFile printer("/dev/usb/lp0"); //if file /dev/usb/lp0 exists then some printer are in plugin&turn on status
    if(printer.exists()){
        res |= 1<<2;
    }

    system("sudo tvservice -d /home/pi/edid/edid_hdmi_plugin_test.txt");
    QFile file("/home/pi/edid/edid_hdmi_plugin_test.txt");
    if(file.exists()){
        system("sudo rm /home/pi/edid/edid_hdmi_plugin_test.txt");
        res |= 1<<1;
    }
    return res;
}

void MainWindow::logDebug(QString msg){
    qDebug()<<"logDebug:"<<msg.toStdString().c_str();
}

void MainWindow::glplay(){
//    GlPlayer* glplayer = new GlPlayer(1024, 638);
////    glplayer->esMainLoop();
//    int width,height;
//    char *buffer = NULL;
//    FILE *f;
//    unsigned char tgaheader[12];
//    unsigned char attributes[6];
//    unsigned int imagesize;

//    f = fopen("jan.tga", "rb");
//    if(f == NULL) return;

//    if(fread(&tgaheader, sizeof(tgaheader), 1, f) == 0)
//    {
//        fclose(f);
//        return;
//    }

//    if(fread(attributes, sizeof(attributes), 1, f) == 0)
//    {
//        fclose(f);
//        return;
//    }

//    width = attributes[1] * 256 + attributes[0];
//    height = attributes[3] * 256 + attributes[2];
//    imagesize = attributes[4] / 8 * width * height;
//    //imagesize *= 4/3;
//    printf("Origin bits: %d\n", attributes[5] & 030);
//    printf("Pixel depth %d\n", attributes[4]);
//    buffer = (char*)malloc(imagesize);
//    if (buffer == NULL)
//    {
//        fclose(f);
//        return;
//    }
//    // invert - should be reflect, easier is 180 rotate
//    int n = 1;
//    while (n <= imagesize) {
//        fread(&buffer[imagesize - n], 1, 1, f);
//        n++;
//    }

//    fclose(f);
//    glplayer->Draw(buffer);
//    sleep(5);
//    glplayer->clear();
}
