#include "videodec.h"
#include <unistd.h>
#include <QApplication>
#include <QDesktopWidget>
#include "glplayer.h"

int read_buffer(void *opaque, uint8_t *buf, int buf_size){
    int available;
    while((available = MainWindow::video_compressed_data_pool->getReadSpace())<=0){
        usleep(50000);
    }
    return MainWindow::video_compressed_data_pool->Read((char*)buf,buf_size>available?available:buf_size);
}

int64_t seek_buffer(void* opaque, int64_t offset, int whence)
{
    int64_t res = MainWindow::video_compressed_data_pool->seek(offset,whence);
    if(res>0){
        qDebug()<<"seek buffer whence"<<whence<<" res "<< res;
    } else {
        qDebug()<<"seek error res:"<<res;
    }
    return res;
}

VideoDec::VideoDec(QObject *parent) :
    QObject(parent),
    exitFlag(false),
    mAudioPlayThread(NULL),
    mAudioPlayer(NULL)
{
}
void VideoDec::init()
{
    play();
}

void VideoDec::play()
{
    int len;
    uint8_t * outbuf = (uint8_t*)malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
    avcodec_register_all();
    av_register_all();
    pFormatCtx = avformat_alloc_context();
    unsigned char *aviobuffer=(unsigned char *)av_malloc(32768);
    avio =avio_alloc_context(aviobuffer, 32768,0,NULL,read_buffer,NULL,NULL);
    pFormatCtx->pb=avio;
    pFormatCtx->flags = AVFMT_FLAG_CUSTOM_IO;

    if(avformat_open_input(&pFormatCtx,"",NULL,NULL)!=0){
        qDebug()<<"OpenFail";
    }

    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        qDebug()<<"FindFail";
    }
    videoindex = -1;
    for(int i=0;pFormatCtx->nb_streams;i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            videoindex = i;
            break;
        }
    }
    if(videoindex == -1){
        qDebug()<<"Didn't find video stream";
    }

    audioindex = -1;
    for(int i=0; i< pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audioindex = i;
            break;
        }
    }
    if(audioindex == -1)
    {
        qDebug()<<"Didn't find a audio stream";
    }

    pCodecCtxVideo = pFormatCtx->streams[videoindex]->codec;
    pCodecVideo = avcodec_find_decoder(pCodecCtxVideo->codec_id);
    if(pCodecVideo == NULL){
        qDebug()<<"codec video not find";
    }
    if(avcodec_open2(pCodecCtxVideo,pCodecVideo,NULL)<0){
        qDebug()<<"can't open codec video";
    }

    pCodecCtxAudio = pFormatCtx->streams[audioindex]->codec;
    pCodecAudio = avcodec_find_decoder(pCodecCtxAudio->codec_id);
    if(!pCodecAudio)
    {
        qDebug()<<"Unsupported codec!";
        return;
    }
    // Open adio codec
    if(avcodec_open2(pCodecCtxAudio, pCodecAudio,NULL)<0)
    {
        qDebug()<<"Could not open codec";
        return;
    }

    av_new_packet(&packet, pCodecCtxVideo->width*pCodecCtxVideo->height);
    SwsContext *convertCtx = sws_getContext(pCodecCtxVideo->width,pCodecCtxVideo->height,pCodecCtxVideo->pix_fmt,
                                            pCodecCtxVideo->width,pCodecCtxVideo->height,PIX_FMT_BGR32,SWS_FAST_BILINEAR,NULL,NULL,NULL);
//    GlPlayer* glplayer = new GlPlayer(pCodecCtxVideo->width,pCodecCtxVideo->height);
    OMXH264Player* h264player = new OMXH264Player();
    qDebug()<<"-------decodec video size:"<<pCodecCtxVideo->width<<"x"<<pCodecCtxVideo->height<<"-------";
    AVBitStreamFilterContext* bsfc = av_bitstream_filter_init("h264_mp4toannexb");
    struct timeval time;
    gettimeofday(&time, NULL);
    qDebug()<<"video end-time:"<< time.tv_sec*1000+time.tv_usec/1000;
    while(!exitFlag && av_read_frame(pFormatCtx,&packet)>=0){
        if(packet.stream_index == videoindex){
            av_bitstream_filter_filter(bsfc, pCodecCtxVideo, NULL, &packet.data, &packet.size, packet.data, packet.size, 0);
            int sendcount = 0;
            while(packet.size>0){
                if(packet.size>81920){
                    h264player->draw(packet.data+sendcount, 81920);
                    sendcount+=81920;
                    packet.size-=81920;
                }else{
                    h264player->draw(packet.data+sendcount, packet.size);
                    packet.size = 0;
                }
            }
        }else if(packet.stream_index == audioindex){
#if 1
            uint8_t *pktdata;
            int pktsize;
            pktdata = packet.data;
            pktsize = packet.size;
            while(pktsize > 0){
                int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
                len = avcodec_decode_audio3(pCodecCtxAudio,(short *)outbuf, &out_size,&packet);
                if (len < 0){
                    qDebug()<<"avcodec_decode_audio3 error len";
                    break;
                }
                if (out_size > 0){
                    while(!exitFlag && MainWindow::mPcmPool->getWriteSpace()<out_size) {
                        usleep(100000);
                    }
                    MainWindow::mPcmPool->Write((char*)outbuf,out_size);
                    if(mAudioPlayer == NULL){
                        mAudioPlayer = new AudioPlayer(2, 44100);
                        mAudioPlayThread = new QThread(this);
                        mAudioPlayer->moveToThread(mAudioPlayThread);
                        connect(this,SIGNAL(audio_play_s()),mAudioPlayer,SLOT(play()));
                        mAudioPlayThread->start();
                        emit audio_play_s();
                    }
                }
                pktsize -= len;
                pktdata += len;
            }
#endif
        }
        av_free_packet(&packet);
    }
    qDebug()<<"-----------------viceo codec loop end-----------------";
    free(outbuf);
    h264player->clear();
    h264player = NULL;
    if(mAudioPlayer != NULL){
        mAudioPlayer->mExitFlag = true;
        mAudioPlayThread->exit();
        mAudioPlayer = NULL;
        mAudioPlayThread = NULL;
    }
    av_bitstream_filter_close(bsfc);
    sws_freeContext(convertCtx);
//    delete glplayer;
}
