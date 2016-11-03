#include "audiodec.h"
extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

int read_buffer_audio(void *opaque, uint8_t *buf, int buf_size){
    while(MainWindow::mDataPool->getReadSpace()<buf_size){
        usleep(500);
//        qDebug()<<"mDataPool no data can read "<<MainWindow::mDataPool->getReadSpace();
    }
    return MainWindow::mDataPool->Read((char*)buf,buf_size);
}

AudioDec::AudioDec(QObject *parent) :
    QObject(parent),
    mExitFlag(false)
{
}
void AudioDec::decode()
{
    qDebug()<<"------------------audio_decode_thread run --------------------";
    avcodec_register_all();/*注册所有的编码解码器*/
    av_register_all();
    AVFormatContext *pFormatCtx;
    AVCodec *aCodec;
    AVCodecContext * aCodecCtx= NULL;
    pFormatCtx = avformat_alloc_context();

    //init AVIOContext for use of read from memory
    unsigned char *aviobuffer=(unsigned char *)av_malloc(4096);
    AVIOContext *avio =avio_alloc_context(aviobuffer, 4096,0,NULL,read_buffer_audio,NULL,NULL);
    pFormatCtx->pb=avio;

    int  len;
    int out_size=AVCODEC_MAX_AUDIO_FRAME_SIZE;
    uint8_t * outbuf;
    int error = 0;
    int audioStream = -1;
    AVPacket packet;
    uint8_t *pktdata;
    int pktsize;

    error = avformat_open_input(&pFormatCtx, NULL, NULL, NULL);
    if(error !=0)
    {
        printf("Couldn't open imput stream, error :%d\n",error);
        return;
    }

    error = avformat_find_stream_info(pFormatCtx,NULL);

    if( error <0)
    {
        printf("Couldn't find stream information error :%d\n",error);
        return;
    }

    //dump_format(pFormatCtx, 0, inputfilename, 0);
    for(int i=0; i< pFormatCtx->nb_streams; i++)
    {

        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            printf("set audio stream to %d\n",i);
            audioStream = i;
        }
    }
    if(audioStream == -1)
    {
        printf("Didn't find a audio stream\n");
    }else printf("find audio stream %d\n",audioStream);

    aCodecCtx=pFormatCtx->streams[audioStream]->codec;
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if(!aCodec)
    {
        fprintf(stderr, "Unsupported codec!\n");
        return;
    }
    // Open adio codec
    if(avcodec_open2(aCodecCtx, aCodec,NULL)<0)
    {
        printf("Could not open codec\n");
        return;
    }

    outbuf = (uint8_t*)malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);

    while(!mExitFlag && av_read_frame(pFormatCtx, &packet) >= 0)
    {
        if(packet.stream_index == audioStream)
        {
            pktdata = packet.data;
            pktsize = packet.size;
            while(pktsize > 0)
            {
                out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
                len = avcodec_decode_audio3(aCodecCtx,(short *)outbuf, &out_size,&packet);
                if (len < 0)
                {
                    printf("error len\n");
                    break;
                }
                if (out_size > 0)
                {
                    while(MainWindow::mPcmPool->getWriteSpace()<out_size) {
                        usleep(2000);
                    }
//                    qDebug()<<"write %d to pool "<<out_size;
                    MainWindow::mPcmPool->Write((char*)outbuf,out_size);
                }
                pktsize -= len;
                pktdata += len;
            }
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
        usleep(2000);
    }
    qDebug()<<"audio decode exit";

    free(outbuf);
    avcodec_close(aCodecCtx);
    av_free(aCodecCtx);
}

