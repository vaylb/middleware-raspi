#include "videodec.h"
#include <unistd.h>
#include <QApplication>
#include <QDesktopWidget>
#include "glplayer.h"

int read_buffer(void *opaque, uint8_t *buf, int buf_size){
//    while(MainWindow::video_compressed_data_pool->getReadSpace()<buf_size){
//        qDebug()<<"----------buffer read have no data----------";
//        usleep(100);
//    }
//    return MainWindow::video_compressed_data_pool->Read((char*)buf,buf_size);

    int available;
    while((available = MainWindow::video_compressed_data_pool->getReadSpace())<=0){
        usleep(50000);
//        qDebug()<<"mDataPool no data can read need"<<buf_size;
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
    exitFlag(false)
{
//    GlPlayer::getPlayer(1920,1080);
}
void VideoDec::init()
{
    play();
}

void VideoDec::play()
{
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
        qDebug()<<"Din't find video stream";
    }

    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL){
        qDebug()<<"codec not find";
    }
    if(avcodec_open2(pCodecCtx,pCodec,NULL)<0){
        qDebug()<<"can't open codec";
    }
    pFrame = avcodec_alloc_frame();
    pFrameRGB = avcodec_alloc_frame();

    int got_picture;
    av_new_packet(&packet, pCodecCtx->width*pCodecCtx->height);

    QDesktopWidget *dwsktopwidget = QApplication::desktop();
    QRect deskrect = dwsktopwidget->availableGeometry();
    uint8_t *out_buffer;
    out_buffer = new uint8_t[avpicture_get_size(PIX_FMT_RGB32, deskrect.width(),deskrect.height())];//分配AVFrame所需内存

//    avpicture_fill((AVPicture *)pFrameRGB, out_buffer, PIX_FMT_RGB32, deskrect.width(),deskrect.height());//填充AVFrame
    avpicture_fill((AVPicture *)pFrameRGB, out_buffer, PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);//填充AVFrame
    avpicture_fill((AVPicture *)pFrame, out_buffer, PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);//填充AVFrame

//    SwsContext *convertCtx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
//                                            deskrect.width(),deskrect.height(),PIX_FMT_RGB32,SWS_FAST_BILINEAR,NULL,NULL,NULL);
    SwsContext *convertCtx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
                                            pCodecCtx->width,pCodecCtx->height,PIX_FMT_BGR32,SWS_FAST_BILINEAR,NULL,NULL,NULL);
    bool needadd = true;
    GlPlayer* glplayer = new GlPlayer(pCodecCtx->width,pCodecCtx->height);
    qDebug()<<"-------decodec video size:"<<pCodecCtx->width<<"x"<<pCodecCtx->height<<"-------";
    while(!exitFlag && av_read_frame(pFormatCtx,&packet)>=0){
        if(packet.stream_index == videoindex){
            int rec = avcodec_decode_video2(pCodecCtx,pFrame,&got_picture,&packet);
            if(rec>0){
                if(needadd){
                    sws_scale(convertCtx,(const uint8_t*  const*)pFrame->data,pFrame->linesize,0
                              ,pCodecCtx->height,pFrameRGB->data,pFrameRGB->linesize);
//                    QImage img((uchar *)pFrameRGB->data[0],deskrect.width(),deskrect.height(),QImage::Format_RGB32);
//                    GlPlayer::getPlayer(pCodecCtx->width,pCodecCtx->height)->Draw((char *)pFrameRGB->data[0]);
                    glplayer->draw((char *)pFrameRGB->data[0]);
#if 0
                    QImage img((uchar *)pFrameRGB->data[0],pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
                    mutex.lock();
                    videoImg.append(img);
                    mutex.unlock();
#endif
                    needadd = false;
                }else{
                    needadd = true;
                }
            }
            av_free_packet(&packet);
        }
        usleep(100);
    }
    sws_freeContext(convertCtx);
    delete glplayer;

//    avcodec_close(pCodecCtx);
//    av_free(pFrame);
//    av_free(pFrameRGB);
//    avio_close(avio);
//    avformat_free_context(pFormatCtx);
}
