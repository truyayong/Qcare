//
// Created by Administrator on 2018/12/15 0015.
//


#include "AvCodecProccessor.h"

AvCodecProccessor::AvCodecProccessor() {
    LOGI("AvCodecProccessor::AvCodecProccessor");
    pthread_mutex_init(&prepareDecodeMutex, NULL);
}

AvCodecProccessor::~AvCodecProccessor() {
    LOGI("AvCodecProccessor::~AvCodecProccessor");
    if (NULL != pAudioCodecCtx) {
        avcodec_close(pAudioCodecCtx);
        avcodec_free_context(&pAudioCodecCtx);
        pAudioCodecCtx = NULL;
    }
    pAudioCodecPara = NULL;

    if (NULL != pVideoCodecCtx) {
        avcodec_close(pVideoCodecCtx);
        avcodec_free_context(&pVideoCodecCtx);
        pVideoCodecCtx = NULL;
    }
    pVideoCodecPara = NULL;

    if (NULL != pAVFormatCtx) {
        avformat_close_input(&pAVFormatCtx);
        avformat_free_context(pAVFormatCtx);
        pAVFormatCtx = NULL;
    }

    pthread_mutex_destroy(&prepareDecodeMutex);
}

void *decodePrepareRunnable(void* data) {
    AvCodecProccessor* pCoder = (AvCodecProccessor*) data;
    pCoder->prepareDecoder();
    pthread_exit(&pCoder->prepareDecodeThread);
}

void AvCodecProccessor::prepare() {
    LOGI("AvCodecProccessor::prepare");
    PlaySession::getIns()->bExit = false;
    pthread_create(&prepareDecodeThread
            , NULL, decodePrepareRunnable, this);
}

/**
 * avformat_open_input打开文件超时可以在这里设置中断
 * @param data
 * @return
 */
int avFormatOpenInterruptCallback(void* data) {
    if (PlaySession::getIns()->bExit) {
        return AVERROR_EOF;
    }
    return 0;
}

void AvCodecProccessor::prepareDecoder() {
    LOGI("AvCodecProccessor::prepareDecoder");
    pthread_mutex_lock(&prepareDecodeMutex);
    av_register_all();
    avformat_network_init();
    pAVFormatCtx = avformat_alloc_context();
    pAVFormatCtx->interrupt_callback.callback = avFormatOpenInterruptCallback;
    pAVFormatCtx->interrupt_callback.opaque = NULL;
    int ret = avformat_open_input(&pAVFormatCtx
            , PlaySession::getIns()->getUrl(), NULL, NULL);
    if (ret != 0) {
        LOGE("AvCodecProccessor::prepareDecoder avformat_open_input err : %s, url : %s", ErrUtil::errLog(ret), PlaySession::getIns()->getUrl());
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, ret, ErrUtil::errLog(ret));
        PlaySession::getIns()->bExit = true;
        pthread_mutex_unlock(&prepareDecodeMutex);
        return;
    }
    ret = avformat_find_stream_info(pAVFormatCtx, NULL);
    if (ret < 0) {
        LOGE("AvCodecProccessor::prepareDecoder avformat_find_stream_info err : %s", ErrUtil::errLog(ret));
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, ret, ErrUtil::errLog(ret));
        PlaySession::getIns()->bExit = true;
        pthread_mutex_unlock(&prepareDecodeMutex);
        return;
    }
    if (initAVCoderctx(&mAudioStreamIndex, &pAudioCodecCtx, &pAudioCodecPara, AVMEDIA_TYPE_AUDIO) != 0) {
        PlaySession::getIns()->bExit = true;
        pthread_mutex_unlock(&prepareDecodeMutex);
        return;
    }
//    if (initAVCoderctx(&mVideoStreamIndex, &pVideoCodecCtx, &pVideoCodecPara, AVMEDIA_TYPE_VIDEO) != 0) {
//        PlaySession::getIns()->bExit = true;
//        pthread_mutex_unlock(&prepareDecodeMutex);
//        return;
//    }
    audioProccessor = new AudioProccessor(pAudioCodecCtx);
    pthread_mutex_unlock(&prepareDecodeMutex);
    NotifyApplication::getIns()->notifyPrepared(CHILD_THREAD);
    LOGI("AvCodecProccessor::prepareDecoder end");
}

int AvCodecProccessor::initAVCoderctx(int *pIndex, AVCodecContext **ppAvCodecCtx,
                                       AVCodecParameters **ppAvCodecpara, AVMediaType type) {
    LOGI("AvCodecProccessor::initAVCoderctx type : %d", type);
    int ret = 0;
    for (int i = 0; i < pAVFormatCtx->nb_streams; i++) {
        if (pAVFormatCtx->streams[i]->codecpar->codec_type == type) {
            *pIndex = i;
            *ppAvCodecpara = pAVFormatCtx->streams[i]->codecpar;
            PlaySession::getIns()->duration = pAVFormatCtx->duration / AV_TIME_BASE;
            PlaySession::getIns()->timeBase = pAVFormatCtx->streams[i]->time_base;
            PlaySession::getIns()->inSampleRate = pAudioCodecPara->sample_rate;
        }
    }

    if (NULL == *ppAvCodecpara) {
        LOGE("AvCodecProccessor::prepareDecoder pAVFormatCtx can not find codecpar");
        ret = -1002;
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, ret, "pAVFormatCtx can not find codecpar");
        //[TODO]truyayong error code
        return ret;
    }
    AVCodec* codec = avcodec_find_decoder((*ppAvCodecpara)->codec_id);
    if (!codec) {
        LOGE("AvCodecProccessor::prepareDecoder avcodec_find_decoder is null ");
        ret = -1000;
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, ret, "avcodec_find_decoder is null");
        //[TODO]truyayong error code
        return ret;
    }
    *ppAvCodecCtx = avcodec_alloc_context3(codec);
    if (!*ppAvCodecCtx) {
        LOGE("AvCodecProccessor::prepareDecoder avcodec_alloc_context3 is null ");
        ret = -1001;
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, ret, "avcodec_alloc_context3 is null");
        //[TODO]truyayong error code
        return ret;
    }
    ret = avcodec_parameters_to_context(*ppAvCodecCtx, *ppAvCodecpara);
    if (ret < 0) {
        LOGE("AvCodecProccessor::prepareDecoder avcodec_parameters_from_context err : %s", ErrUtil::errLog(ret));
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, ret, ErrUtil::errLog(ret));
        return ret;
    }

    ret = avcodec_open2(*ppAvCodecCtx, codec, NULL);
    if (ret < 0) {
        LOGE("AvCodecProccessor::prepareDecoder avcodec_open2 err : %s", ErrUtil::errLog(ret));
        NotifyApplication::getIns()->notifyError(CHILD_THREAD, ret, ErrUtil::errLog(ret));
        return ret;
    }
    LOGI("AvCodecProccessor::initAVCoderctx type : %d end", type);
    return 0;
}

void* startDecodeRunnable(void* data) {
    AvCodecProccessor* pCoder = (AvCodecProccessor*) data;
    if (NULL != pCoder) {
        pCoder->startDecoder();
    }
    pthread_exit(&pCoder->startDecodeThread);
}

void* startPlayRunnable(void* data) {
    AvCodecProccessor* pCoder = (AvCodecProccessor*) data;
    pCoder->audioProccessor->start();
    pthread_exit(&pCoder->startPlayThread);
}

void AvCodecProccessor::start() {
    LOGI("AvCodecProccessor::start");
    PlaySession::getIns()->playState = PLAY_STATE_PLAYING;
    pthread_create(&startDecodeThread, NULL, startDecodeRunnable, this);
    pthread_create(&startPlayThread,NULL, startPlayRunnable, this);
}

void AvCodecProccessor::startDecoder() {
    LOGI("AvCodecProccessor::startDecoder");
    int count = 0;
    while (!PlaySession::getIns()->bExit && NULL != audioProccessor
           && NULL != pAVFormatCtx) {
        if (PlaySession::getIns()->bSeeking) {
            av_usleep(1000 * 100);
            continue;
        }
        if (audioProccessor->pQueue->size() > PacketQueue::MAX_SIZE) {
            av_usleep(1000 * 100);
            continue;
        }
        AVPacket* avPacket = av_packet_alloc();
        int ret = av_read_frame(pAVFormatCtx, avPacket);
        if (ret == 0) {
            if (avPacket->stream_index == mAudioStreamIndex) {
                count++;
                audioProccessor->pQueue->putAvPacket(avPacket);
            } else {
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {
            LOGI("decode finish");
            av_packet_free(&avPacket);
            av_free(avPacket);
            while (!PlaySession::getIns()->bExit) {
                if (audioProccessor->pQueue->size() > 0) {
                    av_usleep(1000 * 100);
                    continue;
                } else {
                    PlaySession::getIns()->bExit = true;
                    break;
                }
            }
        }
    }
    LOGI("AvCodecProccessor::startDecoder end");
}

int AvCodecProccessor::getSampleRate() {
    return 0;
}


void AvCodecProccessor::stop() {
    LOGI("AvCodecProccessor::stop");
    if (NULL != audioProccessor) {
        audioProccessor->stop();
        delete audioProccessor;
        audioProccessor = NULL;
    }
}

void AvCodecProccessor::seek(int64_t second) {
    LOGI("AvCodecProccessor::seek second : %ld duration : %ld", second, PlaySession::getIns()->duration);
    if (PlaySession::getIns()->duration < 0) {
        return;
    }
    if (second >= 0 && second <= PlaySession::getIns()->duration) {
        PlaySession::getIns()->bSeeking = true;
        if (NULL != audioProccessor->pQueue) {
            audioProccessor->pQueue->clearQueue();
        }
        int64_t rel = second * AV_TIME_BASE;
        avcodec_flush_buffers(pAudioCodecCtx);
        avformat_seek_file(pAVFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
        PlaySession::getIns()->bSeeking = false;
    }
}

void AvCodecProccessor::pause() {
    if (NULL != audioProccessor) {
        audioProccessor->pause();
    }
}

void AvCodecProccessor::resume() {
    if (NULL != audioProccessor) {
        audioProccessor->resume();
    }
}

void AvCodecProccessor::setVolume(int percent) {
    if (NULL != audioProccessor) {
        audioProccessor->setVolume(percent);
    }
}

void AvCodecProccessor::switchChannel(int64_t channel) {
    if (NULL != audioProccessor) {
        audioProccessor->switchChannel(channel);
    }
}

void AvCodecProccessor::setPitch(float pitch) {
    if (NULL != audioProccessor) {
        audioProccessor->setPitch(pitch);
    }
}

void AvCodecProccessor::setSpeed(float speed) {
    if (NULL != audioProccessor) {
        audioProccessor->setSpeed(speed);
    }
}


