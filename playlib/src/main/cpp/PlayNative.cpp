#include <jni.h>
#include <string>
#include "AndroidLog.h"
#include "AudioProccessor.h"
#include "PlaySession.h"
#include "NotifyApplication.h"
#include "AvCodecProccessor.h"

AvCodecProccessor* pAvcoder = NULL;
_JavaVM* javaVM = NULL;

/***********************************************************************
 * c++ method
 */
void nativeStop() {
    if (NULL != pAvcoder) {
        pAvcoder->stop();
        delete  pAvcoder;
        pAvcoder = NULL;
    }
    NotifyApplication::getIns()->notifyStopped(MAIN_THREAD);
}

/***********************************************************************
 * jni method
 */
extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    jint result = -1;
    javaVM = vm;
    JNIEnv *env;
    if(vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {

        return result;
    }
    return JNI_VERSION_1_4;

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1prepare(JNIEnv *env, jobject instance,
                                                      jstring source_, jint volume, jint layout) {
    const char *source = env->GetStringUTFChars(source_, 0);
    int length = env->GetStringLength(source_);
    PlaySession::getIns()->allocUrl((char *) source, length);
    NotifyApplication::getIns()->init(javaVM, env, &instance);
    if (NULL == pAvcoder) {
        pAvcoder = new AvCodecProccessor();
    }
    pAvcoder->prepare();
    LOGI("native_prepare url : %s ", PlaySession::getIns()->getUrl());
    env->ReleaseStringUTFChars(source_, source);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1start(JNIEnv *env, jobject instance) {
    if (NULL != pAvcoder) {
        pAvcoder->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1resume(JNIEnv *env, jobject instance) {
    if (NULL != pAvcoder) {
        pAvcoder->resume();
        NotifyApplication::getIns()->notifyResumed(MAIN_THREAD);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1pause(JNIEnv *env, jobject instance) {
    if (NULL != pAvcoder) {
        pAvcoder->pause();
        NotifyApplication::getIns()->notifyPaused(MAIN_THREAD);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1stop(JNIEnv *env, jobject instance) {
    nativeStop();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1seek(JNIEnv *env, jobject instance, jint progress) {
    if (NULL != pAvcoder) {
        pAvcoder->seek(progress);
        NotifyApplication::getIns()->notifySeeked(MAIN_THREAD, progress);
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_playlib_PlayJniProxy_native_1duration(JNIEnv *env, jobject instance) {

    // TODO

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1volume(JNIEnv *env, jobject instance, jint percent) {
    if (NULL != pAvcoder) {
        pAvcoder->setVolume(percent);
        NotifyApplication::getIns()->notifyVolumeModified(MAIN_THREAD, percent);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1pitch(JNIEnv *env, jobject instance, jfloat pitch) {
    if (NULL != pAvcoder) {
        pAvcoder->setPitch(pitch);
        NotifyApplication::getIns()->notifyPitchModified(MAIN_THREAD, pitch);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1speed(JNIEnv *env, jobject instance, jfloat speed) {
    if (NULL != pAvcoder) {
        pAvcoder->setSpeed(speed);
        NotifyApplication::getIns()->notifySpeedModified(MAIN_THREAD, speed);
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_playlib_PlayJniProxy_native_1samplerate(JNIEnv *env, jobject instance) {

    // TODO

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_playlib_PlayJniProxy_native_1channel_1switch(JNIEnv *env, jobject instance,
                                                             jint channel) {
    if (NULL != pAvcoder) {
        pAvcoder->switchChannel(channel);
        NotifyApplication::getIns()->notifyChannelLayoutModified(MAIN_THREAD, channel);
    }
}
