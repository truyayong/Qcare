//
// Created by Administrator on 2018/12/15 0015.
//


#include "AudioProccessor.h"


AudioProccessor::AudioProccessor() {
    pQueue = new PacketQueue();
    pthread_mutex_init(&codecMutex, NULL);
}

AudioProccessor::~AudioProccessor() {
    if (NULL != pQueue) {
        delete pQueue;
    }
    pQueue = NULL;
    if (NULL != buffer) {
        av_free(buffer);
        buffer = NULL;
    }
    pthread_mutex_destroy(&codecMutex);
}

void* startAudioPlayRunnable(void* data) {
    AudioProccessor* pProccessor = (AudioProccessor*) data;
    pProccessor->play();
    return 0;
}

void AudioProccessor::start() {
    LOGI("AudioProccessor::start");
    pthread_create(&startPlayThread,NULL, startAudioPlayRunnable, this);

    LOGI("AudioProccessor::start end");
}

void AudioProccessor::play() {
    prepareSLEngien();
    prepareSLOutputMixAndPlay();
}
void AudioProccessor::pause() {
    LOGI("AudioProccessor::pause");
    PlaySession::getIns()->playState = PLAY_STATE_PAUSED;
    setPlayState(PlaySession::getIns()->playState);
}

void AudioProccessor::resume() {
    LOGI("AudioProccessor::resume");
    PlaySession::getIns()->playState = PLAY_STATE_PLAYING;
    setPlayState(PlaySession::getIns()->playState);
}

void AudioProccessor::stop() {
    LOGI("AudioProccessor::stop");
    if (NULL != pQueue) {
        pQueue->wakeUpQueue();
    }
    pthread_join(startPlayThread, NULL);
    PlaySession::getIns()->playState = PLAY_STATE_STOPPED;
    setPlayState(PlaySession::getIns()->playState);
    if (NULL != pQueue && pQueue->size() > 0) {
        pQueue->clearQueue();
    }
    if (NULL != pAVCodecCtx) {
        avcodec_close(pAVCodecCtx);
        avcodec_free_context(&pAVCodecCtx);
        pAVCodecCtx = NULL;
    }
    pCodecPara = NULL;
    releaseSL();
}

void AudioProccessor::setVolume(int percent) {
    LOGI("AudioProccessor::setVolume percent : %d", percent);
    if (NULL == pcmVolumeItf) {
        return;
    }
    PlaySession::getIns()->volume = percent;
    if (percent > 30)
    {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -20);
    }
    else if (percent > 25)
    {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -22);
    }
    else if (percent > 20)
    {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -25);
    }
    else if (percent > 15)
    {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -28);
    }
    else if (percent > 10)
    {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -30);
    }
    else if (percent > 5)
    {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -34);
    }
    else if (percent > 3)
    {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -37);
    }
    else if (percent > 0)
    {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -40);
    }
    else {
        (*pcmVolumeItf)->SetVolumeLevel(pcmVolumeItf, (100 - percent) * -100);
    }
}

void AudioProccessor::switchChannel(int64_t channel) {
    LOGI("AudioProccessor::switchChannel");
    if (NULL == pcmMuteSoloItf) {
        return;
    }
    if (channel == PLAY_CHANNEL_RIGHT) {//右声道
        (*pcmMuteSoloItf)->SetChannelMute(pcmMuteSoloItf, 1, false);
        (*pcmMuteSoloItf)->SetChannelMute(pcmMuteSoloItf, 0, true);
    } else if (channel == PLAY_CHANNEL_LEFT) {//左声道
        (*pcmMuteSoloItf)->SetChannelMute(pcmMuteSoloItf, 1, true);
        (*pcmMuteSoloItf)->SetChannelMute(pcmMuteSoloItf, 0, false);
    } else if (channel == PLAY_CHANNEL_STEREO) {//立体声
        (*pcmMuteSoloItf)->SetChannelMute(pcmMuteSoloItf, 1, false);
        (*pcmMuteSoloItf)->SetChannelMute(pcmMuteSoloItf, 0, false);
    }
}

void AudioProccessor::setPitch(float pitch) {
}

void AudioProccessor::setSpeed(float speed) {
}

bool AudioProccessor::prepareSLEngien() {
    LOGI("AudioProccessor::prepareSLEngien");
    //TODO[truyayong] 失败之后资源释放问题
    SLresult res;
    res = slCreateEngine(&engineObj, 0, 0, 0, 0, 0);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("slCreateEngine fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "slCreateEngine fail");
        return false;
    }
    res = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("engineObj Realize fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "engineObj Realize fail");
        return false;
    }
    res = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engineItf);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("engineObj GetInterface fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "engineObj GetInterface fail");
        return false;
    }
    return true;
}

bool AudioProccessor::prepareSLOutputMixAndPlay() {
    LOGI("AudioProccessor::prepareSLOutputMixAndPlay");
    //TODO[truyayong] 失败之后资源释放问题
    SLresult res;
    const SLInterfaceID  mids[MIX_ITF_NUM] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean  mreq[MIX_ITF_NUM] = {SL_BOOLEAN_FALSE};
    res = (*engineItf)->CreateOutputMix(engineItf, &outputMixObj, MIX_ITF_NUM, mids, mreq);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("engineItf CreateOutputMix fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "engineItf CreateOutputMix fail");
        return false;
    }
    res = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("outputMixObj Realize fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "outputMixObj Realize fail");
        return false;
    }
    res = (*outputMixObj)->GetInterface(outputMixObj
            , SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("outputMixObj GetInterface fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "outputMixObj GetInterface fail");
        return false;
    }
    res = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
            outputMixEnvironmentalReverb, &reverbSettings);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("outputMixEnvironmentalReverb SetEnvironmentalReverbProperties fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "outputMixEnvironmentalReverb SetEnvironmentalReverbProperties fail");
        return false;
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
    SLDataSink audioSink = {&outputMix, 0};
    prepareSLPlay(audioSink);
    return true;
}

void methodBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void * context) {
    AudioProccessor *pPlayer = (AudioProccessor*) context;
    int pcmSize = 0;
    if (NULL != pPlayer) {
        pcmSize = pPlayer->reSampleAudio((void **)&pPlayer->pOutBuf);
        if (pcmSize > 0) {
            PlaySession::getIns()->audioClock += pcmSize / (double)(PlaySession::getIns()->inSampleRate * 2 * 2);
            if (PlaySession::getIns()->audioClock - PlaySession::getIns()->lastClock >= PlaySession::TIME_INTERVAL) {
                PlaySession::getIns()->lastClock = PlaySession::getIns()->audioClock;
                //TODO[truyayong] 时间回调到应用层
                NotifyApplication::getIns()->notifyProgress(CHILD_THREAD, PlaySession::getIns()->audioClock, PlaySession::getIns()->duration);
            }
        }
        (*pPlayer->pcmBufQueueItf)->Enqueue(pPlayer->pcmBufQueueItf, (char*)pPlayer->pOutBuf
                , pcmSize);
    }
}


bool AudioProccessor::prepareSLPlay(SLDataSink &audioSink) {
    LOGI("AudioProccessor::prepareSLPlay");
    //TODO[truyayong] 失败之后资源释放问题
    //配置pcm格式
    SLDataLocator_AndroidSimpleBufferQueue androidQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            adapterSLSampleRate(PlaySession::getIns()->outSmapleRate),//44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&androidQueue, &pcm};

    SLresult res;
    const SLInterfaceID  ids[PLAY_ITF_NUM] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAYBACKRATE, SL_IID_MUTESOLO};
    const SLboolean req[PLAY_ITF_NUM] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    res = (*engineItf)->CreateAudioPlayer(engineItf, &pcmPlayObj, &slDataSource, &audioSink, PLAY_ITF_NUM, ids, req);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("engineItf CreateAudioPlayer fail code : %d str : %s", res, ErrUtil::errLog(res));
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "engineItf CreateAudioPlayer fail");
        return false;
    }
    res = (*pcmPlayObj)->Realize(pcmPlayObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("pcmPlayObj Realize fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "pcmPlayObj Realize fail");
        return false;
    }

    res = (*pcmPlayObj)->GetInterface(pcmPlayObj, SL_IID_PLAY, &pcmPlayItf);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("pcmPlayObj GetInterface pcmPlayItf fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "pcmPlayObj GetInterface pcmPlayItf fail");
        return false;
    }

    res = (*pcmPlayObj)->GetInterface(pcmPlayObj, SL_IID_VOLUME, &pcmVolumeItf);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("pcmPlayObj GetInterface pcmVolumeItf fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "pcmPlayObj GetInterface pcmVolumeItf fail");
        return false;
    }

    res = (*pcmPlayObj)->GetInterface(pcmPlayObj, SL_IID_MUTESOLO, &pcmMuteSoloItf);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("pcmPlayObj GetInterface pcmMuteSoloItf fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "pcmPlayObj GetInterface pcmMuteSoloItf fail");
        return false;
    }

    res = (*pcmPlayObj)->GetInterface(pcmPlayObj, SL_IID_BUFFERQUEUE, &pcmBufQueueItf);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("pcmPlayObj GetInterface pcmBufQueueItf fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "pcmPlayObj GetInterface pcmBufQueueItf fail");
        return false;
    }

    //TODO[truyayong] 设置缓冲回调接口
    res = (*pcmBufQueueItf)->RegisterCallback(pcmBufQueueItf, methodBufferCallBack, this);
    if (SL_RESULT_SUCCESS != res) {
        LOGE("pcmBufQueueItf RegisterCallback fail code : %d", res);
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, res, "pcmBufQueueItf RegisterCallback fail");
        return false;
    }
    methodBufferCallBack(pcmBufQueueItf, this);

    //TODO[truyayong] 设置播放的初始状态 音量，声道，播放状态等
    setVolume(PlaySession::getIns()->volume);
    switchChannel(PlaySession::getIns()->outChannelLayout);
    setPlayState(PlaySession::getIns()->playState);
    NotifyApplication::getIns()->notifyStarted(CHILD_THREAD);
    return true;
}

void AudioProccessor::setPlayState(int state) {
    LOGI("AudioProccessor::setPlayState");
    if (NULL == pcmPlayItf) {
        return;
    }
    if (state == PLAY_STATE_STOPPED) {
        (*pcmPlayItf)->SetPlayState(pcmPlayItf, SL_PLAYSTATE_STOPPED);
    } else if (state == PLAY_STATE_PAUSED) {
        (*pcmPlayItf)->SetPlayState(pcmPlayItf, SL_PLAYSTATE_PAUSED);
    } else if (state == PLAY_STATE_PLAYING) {
        (*pcmPlayItf)->SetPlayState(pcmPlayItf, SL_PLAYSTATE_PLAYING);
    }
}

int AudioProccessor::adapterSLSampleRate(int sampleRate) {
    int rate = 0;
    switch (sampleRate)
    {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate =  SL_SAMPLINGRATE_44_1;
    }
    return rate;
}

void AudioProccessor::releaseSL() {
    if (NULL != pcmPlayObj) {
        (*pcmPlayObj)->Destroy(pcmPlayObj);
        pcmPlayObj = NULL;
        pcmPlayItf = NULL;
        pcmBufQueueItf = NULL;
        pcmMuteSoloItf = NULL;
        pcmVolumeItf = NULL;
    }

    if (NULL != outputMixObj) {
        (*outputMixObj)->Destroy(outputMixObj);
        outputMixObj = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    if (NULL != engineObj) {
        (*engineObj)->Destroy(engineObj);
        engineObj = NULL;
        engineItf = NULL;
    }

    if (NULL != pOutBuf) {
        pOutBuf = NULL;
    }
}

int AudioProccessor::reSampleAudio(void **pcmBuf) {
    int ret;
    int dataSize = 0;
    AVPacket* avPacket = NULL;
    AVFrame* avFrame = NULL;
    while (!PlaySession::getIns()->bExit) {
        if (PlaySession::getIns()->bSeeking) {
            av_usleep(1000 * 100);
            continue;
        }

        if (pQueue->size() == 0) {
            if (!PlaySession::getIns()->bLoading) {
                PlaySession::getIns()->bLoading = true;
                NotifyApplication::getIns()->notifyLoad(true);
            }
            av_usleep(1000 * 100);
            continue;
        } else {
            if (PlaySession::getIns()->bLoading) {
                PlaySession::getIns()->bLoading = false;
                NotifyApplication::getIns()->notifyLoad(false);
            }
        }

        if (bReadFrameOver) {
            avPacket = av_packet_alloc();
            if (pQueue->getAvPacket(avPacket) != 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                continue;
            }
            pthread_mutex_lock(&codecMutex);
            ret = avcodec_send_packet(pAVCodecCtx, avPacket);
            if (ret != 0) {
                LOGE("avcodec_send_packet ret : %d err: %s", ret, ErrUtil::errLog(ret));
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                pthread_mutex_unlock(&codecMutex);
                continue;
            }
        }
        avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(pAVCodecCtx, avFrame);
        if (ret == 0) {
            bReadFrameOver = false;
            if (avFrame->channels > 0 && avFrame->channel_layout == 0) {
                avFrame->channel_layout = av_get_default_channel_layout(avFrame->channel_layout);
            } else if (avFrame->channels == 0 && avFrame->channel_layout > 0) {
                avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
            }
            PlaySession::getIns()->inChannelLayout = avFrame->channel_layout;
            PlaySession::getIns()->inFmt = (AVSampleFormat)avFrame->format;
            SwrContext* pSwrCtx;
            pSwrCtx = swr_alloc_set_opts(
                    NULL,
                    AV_CH_LAYOUT_STEREO,
                    PlaySession::getIns()->outFmt,
                    avFrame->sample_rate,
                    PlaySession::getIns()->inChannelLayout,
                    PlaySession::getIns()->inFmt,
                    avFrame->sample_rate,
                    NULL, NULL
            );
            if (!pSwrCtx || swr_init(pSwrCtx) < 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
                swr_free(&pSwrCtx);
                bReadFrameOver = true;
                pthread_mutex_unlock(&codecMutex);
                continue;
            }

            if (NULL == buffer) {
                buffer = (uint8_t*) av_malloc(PlaySession::getIns()->inSampleRate
                                              * PlaySession::getIns()->getInChannelLayoutBytes()
                                              * av_get_bytes_per_sample(PlaySession::getIns()->outFmt));
            }
            PlaySession::getIns()->numSampleAvFrame = swr_convert(
                    pSwrCtx, &buffer,
                    avFrame->nb_samples,
                    (const uint8_t **)avFrame->data,
                    avFrame->nb_samples);
            *pcmBuf = buffer;
            int outChannels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            dataSize = PlaySession::getIns()->numSampleAvFrame * outChannels * av_get_bytes_per_sample(PlaySession::getIns()->outFmt);

            double time = avFrame->pts * av_q2d(PlaySession::getIns()->audioTimeBase);
            calcCurrentClock(time);

            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            swr_free(&pSwrCtx);
            pthread_mutex_unlock(&codecMutex);
            break;
        } else {
            bReadFrameOver = true;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }
    }
    return dataSize;
}

void AudioProccessor::calcCurrentClock(double time) {
    if (time < PlaySession::getIns()->audioClock) {
        time = PlaySession::getIns()->audioClock;
    }
    PlaySession::getIns()->audioClock = time;
}




