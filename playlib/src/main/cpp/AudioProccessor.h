//
// Created by Administrator on 2018/12/15 0015.
//
#pragma once
#ifndef QMUSIC_AUDIOPROCCESSOR_H
#define QMUSIC_AUDIOPROCCESSOR_H

#include <stdint.h>
#define MIX_ITF_NUM 1
#define PLAY_ITF_NUM 4

#include <stdint.h>
#include "AndroidLog.h"
#include "NotifyApplication.h"
#include "SoundTouch.h"
#include "PlaySession.h"
#include "PacketQueue.h"
#include "ErrUtil.h"

extern "C" {
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
};
using namespace soundtouch;
/**
 * 音频处理层，负责播放音频与调度AudioCoder解码
 */
class AudioProccessor {
private:
    AVCodecContext* pAVCodecCtx = NULL;
    //opensl 引擎
    SLObjectItf engineObj = NULL;
    SLEngineItf engineItf = NULL;

    //混音器
    SLObjectItf  outputMixObj = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //opensl 播放pcm接口
    SLObjectItf pcmPlayObj = NULL;
    SLPlayItf  pcmPlayItf = NULL;
    SLVolumeItf pcmVolumeItf = NULL;
    SLMuteSoloItf  pcmMuteSoloItf = NULL;


public:
    PacketQueue* pQueue = NULL;
    //开始播放线程
    pthread_t startPlayThread;
    //播放缓冲队列
    SLAndroidSimpleBufferQueueItf  pcmBufQueueItf = NULL;
    uint8_t *pOutBuf = NULL;

    //标识packet是否已经全部解析成frame
    bool bReadFrameOver = true;
    //buffer申请1s的样本数，一般来说一个AVFrame包含的样本数都会小于1s的数量
    uint8_t *buffer = NULL;

    //解码器在多线程下需要同步
    pthread_mutex_t codecMutex;

private:
    void setPlayState(int state);
    int adapterSLSampleRate(int rate);
    //释放OpenSL
    void releaseSL();

    /**
     * 计算当前播放时间
     * @param time
     */
    void calcCurrentClock(double time);

public:
    AudioProccessor(AVCodecContext* pCodecCtx);
    virtual ~AudioProccessor();

    void start();
    void play();
    void pause();
    void resume();
    void stop();
    void setVolume(int percent);
    void switchChannel(int64_t channel);
    void setPitch(float pitch);
    void setSpeed(float speed);

    /**
     * 音频重采样,从packet队列中拿出packet解析成frame，一个packet可能有几个frame,
     * AvCodecProccessor#bReadFrameOver标识packet是否已经全部解析成frame
     * @param pcmBuf
     * @return
     */
    int reSampleAudio(void **pcmBuf);

    //创建引擎
    bool prepareSLEngien();
    //创建混音器和播放器,用于输出音频
    bool prepareSLOutputMixAndPlay();
    //创建播放器
    bool prepareSLPlay(SLDataSink& audioSink);

};


#endif //QMUSIC_AUDIOPROCCESSOR_H
