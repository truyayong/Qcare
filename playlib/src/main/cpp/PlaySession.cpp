//
// Created by Administrator on 2018/12/15 0015.
//

#include "PlaySession.h"

PlaySession::PlaySession() {}

PlaySession *PlaySession::getIns() {
    return p;
}

PlaySession* PlaySession::p = new PlaySession();


char *PlaySession::getUrl() const {
    return this->pUrl;
}

int PlaySession::getInChannelLayoutBytes() {
    int bytes = 0;
    if (inChannelLayout == PLAY_CHANNEL_RIGHT) {//右声道
        bytes = 1;
    } else if (inChannelLayout == PLAY_CHANNEL_LEFT) {//左声道
        bytes = 1;
    } else if (inChannelLayout == PLAY_CHANNEL_STEREO) {//立体声
        bytes = 2;
    }
    return bytes;
}

int PlaySession::getoutChannelLayoutBytes() {
    int bytes = 0;
    if (outChannelLayout == PLAY_CHANNEL_RIGHT) {//右声道
        bytes = 1;
    } else if (outChannelLayout == PLAY_CHANNEL_LEFT) {//左声道
        bytes = 1;
    } else if (outChannelLayout == PLAY_CHANNEL_STEREO) {//立体声
        bytes = 2;
    }
    return bytes;
}

void PlaySession::allocUrl(char *pUrl, int length) {
    if (NULL != this->pUrl) {
        free(this->pUrl);
        this->pUrl = NULL;
    }
    this->pUrl = (char*)malloc(length);
    strcpy(this->pUrl, pUrl);
}

void PlaySession::releaseUrl() {
    if (NULL != this->pUrl) {
        free(this->pUrl);
        this->pUrl = NULL;
    }
}

double PlaySession::getVideoDelayTime() {
    double diff = currentClock - videoClock;
    double delayTime = 0.0;
    if (diff > 0.003) {
        delayTime = delayTime * 2 / 3;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff < -0.003) {
        delayTime = delayTime * 3 / 2;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    }

    if (diff >= 0.2) {
        delayTime = 0;
    } else if (diff <= -0.2) {
        delayTime = defaultDelayTime * 2;
    }

    if (fabs(diff) >= 10) {
        delayTime = defaultDelayTime;
    }
    return delayTime;
}
