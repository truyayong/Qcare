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
    AVCodecContext* pAVCodecCtx = NULL;
public:
    PacketQueue* pQueue = NULL;
    //开始播放线程
    pthread_t startPlayThread;

    //解码器在多线程下需要同步
    pthread_mutex_t codecMutex;
public:
    VideoProccessor(AVCodecContext* pCodecCtx);
    virtual ~VideoProccessor();

    void start();
    void play();
    void stop();

private:
    void calcuVideoClock(AVFrame* avFrame);
};


#endif //QCARE_VIDEOPROCCESSOR_H
