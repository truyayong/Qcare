//
// Created by Administrator on 2019/1/31.
//

#ifndef QCARE_VIDEOPROCCESSOR_H
#define QCARE_VIDEOPROCCESSOR_H


#include "PacketQueue.h"
#include "NotifyApplication.h"
extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
};

class VideoProccessor {
private:

public:
    const static int CODEC_YUV = 0;//支持软解码
    const static int CODEC_MEDIACODEC = 1;//支持硬解码
    AVCodecContext* pAVCodecCtx = NULL;
    PacketQueue* pQueue = NULL;
    //开始播放线程
    pthread_t startPlayThread;

    //解码器在多线程下需要同步
    pthread_mutex_t codecMutex;

    int codecType = CODEC_YUV;
public:
    VideoProccessor(AVCodecContext* pCodecCtx);
    virtual ~VideoProccessor();

    void start();
    void play();
    void stop();

private:
    void calcuVideoClock(AVFrame* avFrame);
    //软解码
    void softDecode(AVPacket* avPacket);
    //硬解码
    void hardDecode(AVPacket* avPacket);
};


#endif //QCARE_VIDEOPROCCESSOR_H
