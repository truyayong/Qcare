//
// Created by Administrator on 2018/12/15 0015.
//

#ifndef QMUSIC_NOTIFYAPPLICATION_H
#define QMUSIC_NOTIFYAPPLICATION_H

#include <jni.h>
#include <cwchar>
#include "AndroidLog.h"

//主进程
#define MAIN_THREAD 0
//子进程
#define CHILD_THREAD 1

/**
 * 通知应用层
 */
class NotifyApplication {
private:
    static NotifyApplication* p;
    _JavaVM* jvm;
    JNIEnv* jenv;
    jobject jobj;
    jmethodID jmid_prepare;
    jmethodID jmid_started;
    jmethodID jmid_resumed;
    jmethodID jmid_paused;
    jmethodID jmid_stopped;
    jmethodID jmid_seeked;
    jmethodID jmid_volumeModified;
    jmethodID jmid_channelLayoutModified;
    jmethodID jmid_pitchModified;
    jmethodID jmid_speedModified;
    jmethodID jmid_progress;
    jmethodID jmid_error;
    jmethodID jmid_renderyuv;
    jmethodID jmid_supportvideo;
    jmethodID jmid_initmediacodec;
    jmethodID jmid_harddecodepacket;
private:
    NotifyApplication();
public:
    static NotifyApplication* getIns();
    void init(_JavaVM *jvm, JNIEnv *jenv, jobject* pObj);
    void notifyPrepared(int type);
    void notifyStarted(int type);
    void notifyResumed(int type);
    void notifyPaused(int type);
    void notifyStopped(int type);
    void notifySeeked(int type, int progress);
    void notifyVolumeModified(int type, int percent);
    void notifyChannelLayoutModified(int type, int layout);
    void notifyPitchModified(int type, float pitch);
    void notifySpeedModified(int type, float speed);
    void notifyLoad(bool load);
    void notifyProgress(int type, float current, int total);
    void notifyError(int type, int code, const char* msg);
    void notifyRenderYUV(int type, int width, int height, uint8_t* fy, uint8_t* fu, uint8_t* fv);
    bool callSupportVideo(int type, const char* ffCodeName);
    void callInitMedaiCodec(int type, const char* mime, int width, int height, int csd0Size, int csd1Size, uint8_t* csd0, uint8_t* csd1);
    void callHardDecodeAvPacket(int type, int size, uint8_t* data);
};


#endif //QMUSIC_NOTIFYAPPLICATION_H
