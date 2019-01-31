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
    int mAudioStreamIndex = -1;
    AVCodecContext* pAudioCodecCtx = NULL;
    AVCodecParameters* pAudioCodecPara = NULL;

    int mVideoStreamIndex = -1;
    AVCodecContext* pVideoCodecCtx = NULL;
    AVCodecParameters* pVideoCodecPara = NULL;


public:
    AudioProccessor* audioProccessor = NULL;
    pthread_t prepareDecodeThread;
    pthread_mutex_t prepareDecodeMutex;

    //开始播放线程
    pthread_t startPlayThread;
    pthread_t startDecodeThread;

private:
    int initAVCoderctx(int* pIndex, AVCodecContext** ppAvCodecCtx, AVCodecParameters** ppAvCodecpara, AVMediaType type);
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
