//
// Created by Administrator on 2018/12/15 0015.
//


#include "AvCodecProccessor.h"

AvCodecProccessor::AvCodecProccessor() {
    LOGI("AvCodecProccessor::AvCodecProccessor");
    pthread_mutex_init(&prepareDecodeMutex, NULL);
    pthread_mutex_init(&seekMutex, NULL);
}

AvCodecProccessor::~AvCodecProccessor() {
    LOGI("AvCodecProccessor::~AvCodecProccessor");

    if (NULL != pAVFormatCtx) {
        avformat_close_input(&pAVFormatCtx);
        avformat_free_context(pAVFormatCtx);
        pAVFormatCtx = NULL;
    }

    pthread_mutex_destroy(&prepareDecodeMutex);
    pthread_mutex_destroy(&seekMutex);
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
    videoProccessor = new VideoProccessor();
    audioProccessor = new AudioProccessor();
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
    if (initAVCoderctx(&audioProccessor->mStreamIndex, &audioProccessor->pAVCodecCtx, &audioProccessor->pCodecPara, AVMEDIA_TYPE_AUDIO) != 0) {
        PlaySession::getIns()->bExit = true;
        pthread_mutex_unlock(&prepareDecodeMutex);
        return;
    }
    if (initAVCoderctx(&videoProccessor->mStreamIndex, &videoProccessor->pAVCodecCtx, &videoProccessor->pCodecPara, AVMEDIA_TYPE_VIDEO) != 0) {
        PlaySession::getIns()->bExit = true;
        pthread_mutex_unlock(&prepareDecodeMutex);
        return;
    }
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
            if (type == AVMEDIA_TYPE_AUDIO) {
                PlaySession::getIns()->duration = pAVFormatCtx->duration / AV_TIME_BASE;
                PlaySession::getIns()->audioTimeBase = pAVFormatCtx->streams[i]->time_base;
                PlaySession::getIns()->inSampleRate = (*ppAvCodecpara)->sample_rate;
            } else if (type == AVMEDIA_TYPE_VIDEO) {
                PlaySession::getIns()->videoTimeBase = pAVFormatCtx->streams[i]->time_base;
                int num = pAVFormatCtx->streams[i]->avg_frame_rate.num;
                int den = pAVFormatCtx->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0) {
                    int fps = num / den;
                    PlaySession::getIns()->defaultDelayTime = 1.0 / fps;
                }
            }
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


void AvCodecProccessor::start() {
    if (NULL == audioProccessor || NULL == videoProccessor) {
        return;
    }

    LOGI("AvCodecProccessor::start ");
    PlaySession::getIns()->playState = PLAY_STATE_PLAYING;
    pthread_create(&startDecodeThread, NULL, startDecodeRunnable, this);
    audioProccessor->start();
    videoProccessor->start();
}

void AvCodecProccessor::startDecoder() {

    const char* codecName = videoProccessor->pAVCodecCtx->codec->name;
    LOGI("AvCodecProccessor::startDecoder codec : %s ", codecName);
    if (NotifyApplication::getIns()->callSupportVideo(CHILD_THREAD, codecName)) {
        LOGE("当前设备支持硬解码当前视频");
        videoProccessor->codecType = VideoProccessor::CODEC_MEDIACODEC;
        initBitStreamFilter(codecName);
    }
    while (!PlaySession::getIns()->bExit && NULL != audioProccessor
           && NULL != pAVFormatCtx && NULL != videoProccessor) {
        if (PlaySession::getIns()->bSeeking) {
            av_usleep(1000 * 100);
            continue;
        }
        if (audioProccessor->pQueue->size() > PacketQueue::MAX_SIZE) {
            av_usleep(1000 * 100);
            continue;
        }
        AVPacket* avPacket = av_packet_alloc();
        pthread_mutex_lock(&seekMutex);
        int ret = av_read_frame(pAVFormatCtx, avPacket);
        pthread_mutex_unlock(&seekMutex);
        if (ret == 0) {
            if (avPacket->stream_index == audioProccessor->mStreamIndex) {
                audioProccessor->pQueue->putAvPacket(avPacket);
            } else if (avPacket->stream_index == videoProccessor->mStreamIndex) {
                videoProccessor->pQueue->putAvPacket(avPacket);
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
                    if (!PlaySession::getIns()->bSeeking) {
                        av_usleep(1000 * 100);
                        PlaySession::getIns()->bExit = true;
                    }
                    break;
                }
            }
        }
    }
    LOGI("AvCodecProccessor::startDecoder end");
}

void AvCodecProccessor::initBitStreamFilter(const char *codecName) {
    if (strcasecmp(codecName, "h264") == 0) {
        bsFilter = av_bsf_get_by_name("h264_mp4toannexb");
    } else if (strcasecmp(codecName, "h265") == 0) {
        bsFilter = av_bsf_get_by_name("hevc_mp4toannexb");
    }

    if (NULL == bsFilter) {
        videoProccessor->codecType = VideoProccessor::CODEC_YUV;
        return;
    }

    if (av_bsf_alloc(bsFilter, &videoProccessor->absCtx) != 0) {
        videoProccessor->codecType = VideoProccessor::CODEC_YUV;
        return;
    }

    if (avcodec_parameters_copy(videoProccessor->absCtx->par_in, videoProccessor->pCodecPara) < 0) {
        videoProccessor->codecType = VideoProccessor::CODEC_YUV;
        av_bsf_free(&videoProccessor->absCtx);
        videoProccessor->absCtx = NULL;
        return;
    }

    if (av_bsf_init(videoProccessor->absCtx) != 0) {
        videoProccessor->codecType = VideoProccessor::CODEC_YUV;
        av_bsf_free(&videoProccessor->absCtx);
        videoProccessor->absCtx = NULL;
        return;
    }

    videoProccessor->absCtx->time_base_in = PlaySession::getIns()->videoTimeBase;
    NotifyApplication::getIns()->callInitMedaiCodec(CHILD_THREAD, codecName, videoProccessor->pAVCodecCtx->width
            , videoProccessor->pAVCodecCtx->height, videoProccessor->pAVCodecCtx->extradata_size
            , videoProccessor->pAVCodecCtx->extradata_size, videoProccessor->pAVCodecCtx->extradata
            , videoProccessor->pAVCodecCtx->extradata);
}

int AvCodecProccessor::getSampleRate() {
    return 0;
}


void AvCodecProccessor::stop() {
    LOGI("AvCodecProccessor::stop");
    PlaySession::getIns()->bExit = true;
    if (NULL != audioProccessor) {
        audioProccessor->stop();
        delete audioProccessor;
        audioProccessor = NULL;
    }

    if (NULL != videoProccessor) {
        videoProccessor->stop();
        delete videoProccessor;
        videoProccessor = NULL;
    }
}

void AvCodecProccessor::seek(int64_t progress) {
    int64_t second = PlaySession::getIns()->duration * (progress / 100.0);
    LOGI("AvCodecProccessor::seek second : %ld duration : %ld", second, PlaySession::getIns()->duration);
    if (PlaySession::getIns()->duration < 0) {
        return;
    }
    if (second >= 0 && second <= PlaySession::getIns()->duration) {
        PlaySession::getIns()->bSeeking = true;
        int64_t rel = second * AV_TIME_BASE;
        pthread_mutex_lock(&seekMutex);
        avformat_seek_file(pAVFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
        if (NULL != audioProccessor) {
            if (NULL != audioProccessor->pQueue) {
                audioProccessor->pQueue->clearQueue();
            }
            PlaySession::getIns()->audioClock = 0;
            PlaySession::getIns()->lastClock = 0;

            pthread_mutex_lock(&audioProccessor->codecMutex);
            avcodec_flush_buffers(audioProccessor->pAVCodecCtx);
            pthread_mutex_unlock(&audioProccessor->codecMutex);
        }

        if (NULL != videoProccessor) {
            if (NULL != videoProccessor->pQueue) {
                videoProccessor->pQueue->clearQueue();
            }
            PlaySession::getIns()->videoClock = 0;

            pthread_mutex_lock(&videoProccessor->codecMutex);
            avcodec_flush_buffers(videoProccessor->pAVCodecCtx);
            pthread_mutex_unlock(&videoProccessor->codecMutex);
        }
        pthread_mutex_unlock(&seekMutex);
        PlaySession::getIns()->bSeeking = false;
    }
}

void AvCodecProccessor::pause() {
    if (NULL != audioProccessor) {
        audioProccessor->pause();
    }
    PlaySession::getIns()->bPause = true;
}

void AvCodecProccessor::resume() {
    if (NULL != audioProccessor) {
        audioProccessor->resume();
    }
    PlaySession::getIns()->bPause = false;
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



