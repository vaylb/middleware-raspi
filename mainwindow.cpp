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

DataBuffer* MainWindow::video_compressed_data_pool = new DataBuffer(32768*400);//12.5M
DataBuffer* MainWindow::mDataPool = new DataBuffer(1024*4*16);
DataBuffer* MainWindow::mPcmPool = new DataBuffer(AVCODEC_MAX_AUDIO_FRAME_SIZE);

MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent),
    startflag(false),
    bytesReceived(0),
    setupbytesReceived(0),
    mJpegResize(NULL),
    mShowJpegFlag(true),
    mSetupFlag(false),
    mAudioStartFlag(false),
    mAudioPlayBackFlag(false)
{
//    setenv("EGL_FORCE_DRI3","1",1);
    videoShow = new QLabel;
    videoShowTips = new QLabel;
    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(videoShow, 0, Qt::AlignCenter|Qt::AlignBottom);
    videoShow->setText("中间件系统");
    videoShow->setStyleSheet("font-size:40px");
    mainLayout->addWidget(videoShowTips,0, Qt::AlignCenter|Qt::AlignTop);
    videoShowTips->setText("扫描网络连接...");
    videoShowTips->setStyleSheet("font-size:20px");
    setLayout(mainLayout);
    setWindowTitle("中间件系统");
    setWindowFlags(Qt::FramelessWindowHint);//去掉标题栏*/
    QDesktopWidget *dwsktopwidget = QApplication::desktop();
    QRect deskrect = dwsktopwidget->availableGeometry();
    setFixedSize(deskrect.width(),deskrect.height()); //设置窗体固定大小
//    gl = new CPlayWidget(this);
//    gl->PlayOneFrame();
//    mainLayout->addWidget(gl, 0, Qt::AlignCenter|Qt::AlignBottom);
    printscreeninfo();

    startListening();
//    int i =0;
//    int t =0;
//    system("sudo cp  /etc/wpa_supplicant/wpa_supplicant.conf ./wpa_supplicant_temp.conf");
//    system("sudo iwlist wlan0 scan >./temp ");
//    system("grep -E \"SSID|Quality\" temp >./grepTemp");
//    system("grep -v \"x00\" grepTemp >./temp");
//    QString fileName;
//    fileName = "/home/pi/middleware/video/build-middleware-qt-Desktop-Debug/temp";
//    QFile file(fileName);
//    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
//    {
//        qDebug()<<"!";
//        return;
//    }
//    QTextStream in(&file);
//    QString line = in.readLine();
//    while (i<20)
//    {
//        line = in.readLine();
//        for(t=27;t<line.size()-1;t++)       //take off  other words
//        {
//            wifiName[i] +=line[t];
//        }
//        line = in.readLine();
//        for(t=28;t<30;t++)       //take off  other words
//        {
//            temp[i] +=line[t];
//        }
//        i++;
//    }
//    for(i=0;i<20;i++)
//    {
//        if(wifiName[i].compare("Middleware") == 0){
//            QString wifiConnect = tr("sudo iwconfig wlan0 essid \"%1\" ").arg(wifiName[i]);
//            qDebug()<<wifiConnect;
//            system(wifiConnect.toStdString().c_str());
//            break;
//        }
//    }
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
        qDebug()<<"device scan, receive:"<<datagram.data()<<", ip:"<<host_ip;
        if(strcmp(datagram.data(),"a")==0){
            //response to device scan
            QUdpSocket *device_scan_join_up = new QUdpSocket(this);
            char mac[17];
            get_mac(mac);
            QByteArray msg = "{\"name\":\"video_player_01\",\"type\":1}"; //Fixme
            msg.replace(22,2,mac);
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
            videoShow->setText("");
            mainLayout->setAlignment(videoShow,Qt::AlignCenter);
            videoShowTips->setHidden(true);
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
            mKAudioType = TYPE_AAC;
            mAudioPlayer = new AudioPlayer(2, 44100);
            mAudioDec = new AudioDec();
            audio_data_receiver->connectToHost(*hostaddr, audio_data_port);
        }else if(!mAudioPlayBackFlag && strcmp(datagram.data(),"e")==0){
            videoShowTips->setText("播放音乐...");
            mAudioPlayBackFlag  = true;
            mKAudioType = TYPE_PCM;
            mPcmPool = mDataPool;
            mAudioPlayer = new AudioPlayer(1, 48000);
            audio_data_receiver->connectToHost(*hostaddr, audio_data_port);
        }
        else if(strcmp(datagram.data(),"f")==0){
            mShowJpegFlag = false;
            mAudioPlayBackFlag = false;

            QDesktopWidget *dwsktopwidget = QApplication::desktop();
            QRect deskrect = dwsktopwidget->availableGeometry();
            videoShow->setFixedWidth(deskrect.width());
            videoShow->setFixedHeight(deskrect.height()/2);
            videoShow->setText("中间件系统");
            videoShow->setAlignment(Qt::AlignCenter|Qt::AlignBottom);
            videoShowTips->setHidden(false);
            videoShowTips->setText("播放结束");
            videoShowTips->setAlignment(Qt::AlignCenter|Qt::AlignTop);
//            qDebug()<<video_compressed_data_receiver->state();
//            qDebug()<<video_data_receiver->state();
//            qDebug()<<audio_data_receiver->state();
//            if(videoDecThread != NULL && videoDecThread->isRunning()){
//                mVideoDec->exitFlag = true;
//            }
//            if(audioPlayThread != NULL && audioPlayThread->isRunning()){
//                   mAudioPlayer->mExitFlag = true;
//            }
//            if(audioDecodeThread != NULL && audioDecodeThread->isRunning()){
//                mAudioDec->mExitFlag = true;
//            }
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
    qint32 available = video_compressed_data_receiver->bytesAvailable();
    qint32 data_pool_can_write = video_compressed_data_pool->getWriteSpace();

    if(data_pool_can_write==0) {
//        qDebug()<<"new data but datapool can not write";
        return;
    }

    qint32 read_temp = available>data_pool_can_write?data_pool_can_write:available;
    inBlock = video_compressed_data_receiver->read(read_temp);
    video_compressed_data_pool->Write(inBlock.data(),read_temp);
    inBlock.resize(0);
    //qDebug() <<"data available:"<<available<<", datapool has:"<<video_compressed_data_pool->getReadSpace();

    if(!startflag && video_compressed_data_pool->getReadSpace()>1024*256){
        startflag = true;
        videoDecThread = new QThread(this);
        mVideoDec = new VideoDec();
        readTimer = new QTimer(this);
        mVideoDec->moveToThread(videoDecThread);

        connect(this,SIGNAL(init_s()),mVideoDec,SLOT(init()));
        connect(this,SIGNAL(play_s()),mVideoDec,SLOT(play()));
        connect(readTimer,SIGNAL(timeout()),this,SLOT(showVideo()));
        qDebug()<<"emit start signal";
        videoDecThread->start();
        emit init_s();
        readTimer->start(35);
    }
}

void MainWindow::showVideo()
{
    QPixmap pix;
    if(mVideoDec->exitFlag || mVideoDec->videoImg.isEmpty()) {
        if(mVideoDec->exitFlag){
            qDebug()<<"video show stop";
            readTimer->stop();
            mVideoDec->videoImg.clear();
        }
//        qDebug()<<"no image";
        return;
    }
    mVideoDec->mutex.lock();
    pix = pix.fromImage(mVideoDec->videoImg.dequeue());
    mVideoDec->mutex.unlock();
    videoShow->setFixedSize(pix.width(),pix.height());
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
    qint32 available = audio_data_receiver->bytesAvailable();
    qint32 data_pool_can_write  = 0;
    while((data_pool_can_write = mDataPool->getWriteSpace()) <=0){
        usleep(2000);
    }

    if(data_pool_can_write==0) {
//        qDebug()<<"new data but datapool can not write, can read "<<mDataPool->getReadSpace();
        return;
    }

    qint32 read_temp = available>data_pool_can_write?data_pool_can_write:available;
    if(read_temp>4096){
        read_temp = 4096;
    }
    audioInBlock = audio_data_receiver->read(read_temp);
    mDataPool->Write(audioInBlock.data(),read_temp);
    audioInBlock.resize(0);
    //qDebug() <<"data available:"<<available<<", datapool has:"<<video_compressed_data_pool->getReadSpace();

    if(!mAudioStartFlag && mDataPool->getReadSpace()>4096*8){
        mAudioStartFlag = true;
        audioPlayThread = new QThread(this);
//        mAudioPlayer = new AudioPlayer();
        mAudioPlayer->moveToThread(audioPlayThread);

        connect(this,SIGNAL(audio_play_s()),mAudioPlayer,SLOT(play()));
        audioPlayThread->start();
        emit audio_play_s();

        if(mKAudioType == TYPE_AAC){
            audioDecodeThread = new QThread(this);
//            mAudioDec = new AudioDec();
            mAudioDec->moveToThread(audioDecodeThread);
            connect(this,SIGNAL(decode_s()),mAudioDec,SLOT(decode()));
            audioDecodeThread->start();
            emit decode_s();
        }
    }
}

void MainWindow::printscreeninfo()

{
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
int MainWindow::get_mac(char* mac)
{
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
