#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QDialog>
#include <QLabel>
#include <qlayout.h>
#include "videodec.h"
#include "DataBuffer.h"
#include "jpegresize.h"
#include "audioplayer.h"
#include "audiodec.h"

class VideoDec;
class JpegResize;
class AudioPlayer;
class AudioDec;

namespace Ui {
class MainWindow;
}

enum AudioType
{
    TYPE_AAC = 1,
    TYPE_PCM = 2
};
enum DeviceType{
    DEVICE_AUDIO = 0,
    DEVICE_VIDEO = 1,
    DEVICE_PRINTER = 2
};

class MainWindow : public QDialog
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int get_mac(char* mac);

private slots:
    void on_pushButton_clicked();
    void sendMessage(QString msg);

    void device_scan_come(); //scan
    void displayError(QAbstractSocket::SocketError);
    void printscreeninfo();
    void startListening();

//video jpeg data
    void receive_video_data();
    void showJpeg(int width, int height);    //show jpeg data
    void slotClose();
    int getDeviceType();
    QString getJobDoneMsg();
    void logDebug(QString msg);

signals:
    void resize_s();      //jpeg data


//video compressed data
signals:
    void init_s();
    void play_s();
public slots:
    void showVideo();
    void receive_compressed_video_data();

//setup
    void receive_setup_data();

// audio
    void receive_audio_data();
signals:
    void audio_play_s();
    void decode_s();

//file print
public slots:
    void receive_file_data();

private:
    Ui::MainWindow *ui;
    QVBoxLayout *mainLayout;
    QPushButton *closeButton;
    QString mHostAddr;

    int device_scan_port;
    QUdpSocket *device_scan_receiver;

    //for use of jpeg data
    int video_data_port;
    QTcpSocket *video_data_receiver;
    quint32  TotalBytes;                   //总共需接收的字节数
    quint32  bytesReceived;                //已接收字节数
    QByteArray imagedata;
    QBuffer *imagebuffer;
    QFile *localFile;                     //待接收文件
    QByteArray inBlock;
    QDataStream *instream;
    JpegResize * mJpegResize;
    QThread *jpegResizeThread;
    volatile bool mShowJpegFlag;

    //fpr use of compressed video data, .mp4 etc
    int video_compressed_data_port;
    QTcpSocket *video_compressed_data_receiver;
    QThread *videoDecThread;
    VideoDec *mVideoDec;
    QString FileName;
    QTimer *readTimer;
    QLabel *videoShow;
    QLabel *videoShowTips;
//    CPlayWidget *gl;
    bool startflag;

    //setup
    int device_setup_port;
    QTcpSocket *device_setup_receiver;
    quint32  setupTotalBytes;
    quint32  setupbytesReceived;
    QDataStream *setupinstream;
    QByteArray setupdata;
    QBuffer *setupbuffer;
    QByteArray setupinBlock;
    bool mSetupFlag;

    //audio
    volatile enum AudioType mKAudioType;
    int audio_data_port;
    QTcpSocket *audio_data_receiver;
    QByteArray audioInBlock;
    volatile bool mAudioStartFlag;
    QThread *audioPlayThread;
    QThread *audioDecodeThread;
    AudioPlayer * mAudioPlayer;
    AudioDec * mAudioDec;
    volatile bool mAudioPlayBackFlag;
    volatile bool mAudioDataFileFlag; //为true需要接收定量数据
    quint32  mAudioDataFileTotalBytes;
    quint32  mAudioDataFileReceived;

    //file print
    int file_data_port;
    QTcpSocket *file_data_receiver;
    quint32  filetotalbytes;                   //总共需接收的字节数
    quint32  filebytesreceived;                //已接收字节数
    QByteArray filedata;
    QBuffer *filebuffer;
//    QFile *localFile;                     //待接收文件
    QByteArray fileinblock;
    QDataStream *fileinstream;


public:
    static DataBuffer *video_compressed_data_pool; //holds video data
    static DataBuffer *mDataPool; //holds audio data that are not decoded
    static DataBuffer *mPcmPool; //holds audio pcm data that are decoded from mDataPool

private:
    QString transfer;
    QString temp[20];
    QString wifiName[20];

};

#endif // MAINWINDOW_H
