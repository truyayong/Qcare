//
// Created by Administrator on 2018/12/15 0015.
//

#ifndef QMUSIC_AUDIOCODER_H
#define QMUSIC_AUDIOCODER_H

#include <stdint.h>
#include <pthread.h>
#include "ErrUtil.h"
#include "PlaySession.h"
#include "NotifyApplication.h"
#include "PacketQueue.h"
#include "AudioProccessor.h"
#include "VideoProccessor.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
};

/**
 * 音频编解码层
 */
class AvCodecProccessor {
private:
    AVFormatContext* pAVFormatCtx = NULL;


    const AVBitStreamFilter* bsFilter = NULL;

public:
    AudioProccessor* audioProccessor = NULL;
    VideoProccessor* videoProccessor = NULL;
    pthread_t prepareDecodeThread;
    pthread_mutex_t prepareDecodeMutex;
    //seek同步锁
    pthread_mutex_t seekMutex;

    //开始解码线程
    pthread_t startDecodeThread;

private:
    int initAVCoderctx(int* pIndex, AVCodecContext** ppAvCodecCtx, AVCodecParameters** ppAvCodecpara, AVMediaType type);
    void initBitStreamFilter(const char* codecName);
public:
    AvCodecProccessor();
    virtual ~AvCodecProccessor();

    void prepare();
    void prepareDecoder();
    void start();
    void startDecoder();
    void pause();
    void resume();
    int getSampleRate();
    void stop();
    void seek(int64_t second);
    void setVolume(int percent);
    void switchChannel(int64_t channel);
    void setPitch(float pitch);
    void setSpeed(float speed);

};


#endif //QMUSIC_AUDIOCODER_H
