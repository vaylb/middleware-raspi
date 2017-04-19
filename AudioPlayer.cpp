#include "audioplayer.h"


AudioPlayer::AudioPlayer(int channels, int frequency, int framebits, QObject *parent) :
    QObject(parent),
    mExitFlag(false),
    mAudioChannels(channels),
    mAudioFrequency(frequency),
    mAudioFramebits(framebits)
{
}
void AudioPlayer::play()
{
//    qDebug()<<"-------------------- audio_play_thread start --------------------";
    int rc;
    int ret;
    int size;
    snd_pcm_t* handle; //PCI设备句柄
    snd_pcm_hw_params_t* params;//硬件信息和PCM流配置
    unsigned int val;
    int dir=0;
    snd_pcm_uframes_t frames;
    char *buffer;
    int channels = mAudioChannels;
    int frequency = mAudioFrequency;
    int bit = mAudioFramebits;
    rc=snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if(rc<0)
    {
        qDebug()<<"open PCM device failed";
        exit(1);
    }

    snd_pcm_hw_params_alloca(&params); //分配params结构体
    if(rc<0)
    {
        qDebug()<<"\nsnd_pcm_hw_params_alloca";
        exit(1);
    }

    rc=snd_pcm_hw_params_any(handle, params);//初始化params
    if(rc<0)
    {
        qDebug()<<"\nsnd_pcm_hw_params_any";
        exit(1);
    }

    rc=snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED); //初始化访问权限
    if(rc<0)
    {
        qDebug()<<"\nsed_pcm_hw_set_access";
        exit(1);
    }

    //采样位数
    switch(bit/8)
    {
    case 1:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
            break ;
    case 2:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
            break ;
    case 3:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S24_LE);
            break ;
    }

    rc=snd_pcm_hw_params_set_channels(handle, params, channels); //设置声道,1表示单声道，2表示立体声
    if(rc<0)
    {
        qDebug()<<"snd_pcm_hw_params_set_channels:";
        exit(1);
    }

    val = frequency;
    rc=snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir); //设置频率
    if(rc<0)
    {
        qDebug()<<"snd_pcm_hw_params_set_rate_near";
        exit(1);
    }

    rc = snd_pcm_hw_params(handle, params);
    if(rc<0)
    {
        qDebug()<<"snd_pcm_hw_params";
        exit(1);
    }

    rc=snd_pcm_hw_params_get_period_size(params, &frames, &dir); /*获取周期长度*/
    if(rc<0)
    {
        qDebug()<<"snd_pcm_hw_params_get_period_size";
        exit(1);
    }

    if(channels == 1) size = frames * 2;
    else size = frames * 4; /*4 代表数据快长度*/
//    qDebug()<<"snd_pcm buffer size:"<<size;
    buffer =(char*)malloc(size);
    //fseek(fp,58,SEEK_SET); //定位歌曲到数据区
    int count = 0;
    struct timeval time;
    gettimeofday(&time, NULL);
    qDebug()<<"audio end-time:"<< time.tv_sec*1000+time.tv_usec/1000;
    while (!mExitFlag)
    {
        memset(buffer,0,sizeof(buffer));
        while(!mExitFlag && MainWindow::mPcmPool->getReadSpace()<size){
//            qDebug()<<"AudioPlayer pcmpool drain";
            usleep(100000);
        }
        if(mExitFlag){
            break;
        }
        count++;
        ret = MainWindow::mPcmPool->Read(buffer,size);
        if(ret == 0)
        {
            qDebug()<<"歌曲写入结束";
            break;
        }
        else if (ret != size)
        {
             qDebug()<<"ret != size";
        }
        // 写音频数据到PCM设备
        while((ret = snd_pcm_writei(handle, buffer, frames))<0)
        {
            usleep(4000);
            if (ret == -EPIPE)
            {
                /* EPIPE means underrun */
                qDebug()<<"underrun occurred";
                //完成硬件参数设置，使设备准备好
                snd_pcm_prepare(handle);
            }else if(ret == -EBADFD){
                qDebug()<<"EBADFD";
            }else if(ret == -ESTRPIPE){
                qDebug()<<"ESTRPIPE";
            }
            else
            {
                qDebug()<<"error from writei:"<<snd_strerror(ret)<<ret;
            }
        }
    }

    qDebug()<<"--------------audio player exit--------------";
    // snd_pcm_drain(handle);
    snd_pcm_drop(handle);
    snd_pcm_close(handle);
    free(buffer);
    return;
}
