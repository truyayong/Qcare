//
// Created by Administrator on 2019/1/31.
//

#include "VideoProccessor.h"


VideoProccessor::VideoProccessor(AVCodecContext* pCodecCtx) {
    pQueue = new PacketQueue();
    pAVCodecCtx = pCodecCtx;
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
            av_usleep(1000* 100);
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

        ret = avcodec_send_packet(pAVCodecCtx, avPacket);
        if (ret != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
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
            continue;
        }
        if (avFrame->format == AV_PIX_FMT_YUV420P) {
            LOGI("当前视频是YUV420P格式");
            NotifyApplication::getIns()->notifyRenderYUV(CHILD_THREAD, pAVCodecCtx->width
                    , pAVCodecCtx->height, avFrame->data[0], avFrame->data[1], avFrame->data[2]);
        } else {
            LOGI("当前视频不是YUV420P格式");
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
    }
}

void VideoProccessor::stop() {

}