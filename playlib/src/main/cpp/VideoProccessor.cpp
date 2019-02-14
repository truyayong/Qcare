//
// Created by Administrator on 2019/1/31.
//

#include "VideoProccessor.h"


VideoProccessor::VideoProccessor(AVCodecContext* pCodecCtx) {
    pQueue = new PacketQueue();
    pAVCodecCtx = pCodecCtx;
    pthread_mutex_init(&codecMutex, NULL);
}

VideoProccessor::~VideoProccessor() {
    if (NULL != pQueue) {
        pQueue->clearQueue();
        delete pQueue;
        pQueue = NULL;
    }

    if (NULL != pAVCodecCtx) {
        pAVCodecCtx = NULL;
    }
    pthread_mutex_destroy(&codecMutex);
}

void* startVedioPlayRunnable(void* data) {
    VideoProccessor* pProccessor = static_cast<VideoProccessor*>(data);
    pProccessor->play();
    pthread_exit(&pProccessor->startPlayThread);
}

void VideoProccessor::start() {
    LOGI("VideoProccessor::start");
    pthread_create(&startPlayThread,NULL, startVedioPlayRunnable, this);
    LOGI("VideoProccessor::start end");
}

void VideoProccessor::play() {
    LOGI("VideoProccessor::play");
    int ret;
    while (!PlaySession::getIns()->bExit) {
        if (PlaySession::getIns()->bSeeking) {
            av_usleep(1000 * 100);
            continue;
        }

        if (PlaySession::getIns()->bPause) {
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
        AVPacket* avPacket = av_packet_alloc();
        if (pQueue->getAvPacket(avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }

        pthread_mutex_lock(&codecMutex);
        ret = avcodec_send_packet(pAVCodecCtx, avPacket);
        if (ret != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }
        AVFrame* avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(pAVCodecCtx, avFrame);
        if (ret != 0) {
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }

        //音视频同步sleep
        calcuVideoClock(avFrame);
        av_usleep(PlaySession::getIns()->getVideoDelayTime() * 1000000);

        if (avFrame->format == AV_PIX_FMT_YUV420P) {
            NotifyApplication::getIns()->notifyRenderYUV(CHILD_THREAD, pAVCodecCtx->width
                    , pAVCodecCtx->height, avFrame->data[0], avFrame->data[1], avFrame->data[2]);
        } else {
            AVFrame *pFrameYUV420P = av_frame_alloc();
            int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P
                    , pAVCodecCtx->width, pAVCodecCtx->height, 1);

            uint8_t* buffer = static_cast<uint8_t *>(av_malloc(bufferSize * sizeof(uint8_t)));

            av_image_fill_arrays(pFrameYUV420P->data, pFrameYUV420P->linesize, buffer
                    , AV_PIX_FMT_YUV420P, pAVCodecCtx->width, pAVCodecCtx->height, 1);

            SwsContext* swsCtx = sws_getContext(pAVCodecCtx->width, pAVCodecCtx->height
                    , pAVCodecCtx->pix_fmt, pAVCodecCtx->width, pAVCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

            if (!swsCtx) {
                av_frame_free(&pFrameYUV420P);
                av_free(pFrameYUV420P);
                pFrameYUV420P = NULL;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                pthread_mutex_unlock(&codecMutex);
                continue;
            }
            sws_scale(swsCtx, reinterpret_cast<const uint8_t *const *>(avFrame->data)
                    , avFrame->linesize, 0, avFrame->height, pFrameYUV420P->data, pFrameYUV420P->linesize);

            NotifyApplication::getIns()->notifyRenderYUV(CHILD_THREAD, pAVCodecCtx->width
                    , pAVCodecCtx->height, pFrameYUV420P->data[0], pFrameYUV420P->data[1], pFrameYUV420P->data[2]);

            av_frame_free(&pFrameYUV420P);
            av_free(pFrameYUV420P);
            pFrameYUV420P = NULL;
            av_free(buffer);
            buffer = NULL;
            sws_freeContext(swsCtx);
        }
        av_frame_free(&avFrame);
        av_free(avFrame);
        avFrame = NULL;
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
        pthread_mutex_unlock(&codecMutex);
    }
}

void VideoProccessor::stop() {
    if (NULL != pQueue && pQueue->size() > 0) {
        pQueue->clearQueue();
    }
}

void VideoProccessor::calcuVideoClock(AVFrame* avFrame) {
    if (NULL == avFrame) {
        return;
    }
    double pts = av_frame_get_best_effort_timestamp(avFrame);
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts *= av_q2d(PlaySession::getIns()->videoTimeBase);
    if (pts > 0) {
        PlaySession::getIns()->videoClock = pts;
    }
}
