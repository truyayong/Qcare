//
// Created by Administrator on 2018/12/15 0015.
//

#include "NotifyApplication.h"

NotifyApplication::NotifyApplication() {}

NotifyApplication* NotifyApplication::getIns() {
    return p;
}

NotifyApplication* NotifyApplication::p = new NotifyApplication();

void NotifyApplication::init(_JavaVM *jvm, JNIEnv *jenv, jobject *pObj) {
    this->jvm = jvm;
    this->jenv = jenv;
    this->jobj = *pObj;
    this->jobj = jenv->NewGlobalRef(jobj);

    jclass jlz = jenv->GetObjectClass(this->jobj);
    if (!jlz) {
        LOGE("get jclass wrong");
        return;
    }

    this->jmid_error = jenv->GetMethodID(jlz, "onError", "(ILjava/lang/String;)V");
    this->jmid_prepare = jenv->GetMethodID(jlz, "onPrepared", "()V");
    this->jmid_started = jenv->GetMethodID(jlz, "onStarted", "()V");
    this->jmid_resumed = jenv->GetMethodID(jlz, "onResumed", "()V");
    this->jmid_paused = jenv->GetMethodID(jlz, "onPaused", "()V");
    this->jmid_stopped = jenv->GetMethodID(jlz, "onStopped", "()V");
    this->jmid_seeked = jenv->GetMethodID(jlz, "onSeeked", "(I)V");
    this->jmid_volumeModified = jenv->GetMethodID(jlz, "onVolumeModified", "(I)V");
    this->jmid_channelLayoutModified = jenv->GetMethodID(jlz, "onChannelLayoutModify", "(I)V");
    this->jmid_pitchModified = jenv->GetMethodID(jlz, "onPitchModified", "(F)V");
    this->jmid_speedModified = jenv->GetMethodID(jlz, "onSpeedModified", "(F)V");
    this->jmid_progress = jenv->GetMethodID(jlz, "onPlayProgress", "(FI)V");
    this->jmid_renderyuv = jenv->GetMethodID(jlz, "onRenderYUV", "(II[B[B[B)V");
    this->jmid_supportvideo = jenv->GetMethodID(jlz, "onSupportMediaCodec", "(Ljava/lang/String;)Z");
    this->jmid_initmediacodec = jenv->GetMethodID(jlz, "onInitMediaCodec", "(Ljava/lang/String;II[B[B)V");
    this->jmid_harddecodepacket = jenv->GetMethodID(jlz, "hardDecodeAvPacket", "(I[B)V");
}

void NotifyApplication::notifyError(int type, int code, const char *msg) {
    if (MAIN_THREAD == type) {
        jstring jmsg = jenv->NewStringUTF(msg);
        jenv->CallVoidMethod(jobj, jmid_error, code, jmsg);
        jenv->DeleteLocalRef(jmsg);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyError get child jnienv wrong");
            return;
        }
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(jobj, jmid_error, code, jmsg);
        env->DeleteLocalRef(jmsg);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyPrepared(int type) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_prepare);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyPrepared get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_prepare);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyLoad(bool load) {

}

void NotifyApplication::notifyStopped(int type) {

    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_stopped);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyStopped get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_stopped);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyStarted(int type) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_started);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyStarted get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_started);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyResumed(int type) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_resumed);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyStarted get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_resumed);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyPaused(int type) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_paused);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyStarted get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_paused);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifySeeked(int type, int progress) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_seeked, progress);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifySeeked get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_seeked, progress);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyVolumeModified(int type, int percent) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_volumeModified, percent);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyVolumeModified get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_volumeModified, percent);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyChannelLayoutModified(int type, int layout) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_channelLayoutModified, layout);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyChannelLayoutModified get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_channelLayoutModified, layout);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyPitchModified(int type, float pitch) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_pitchModified, pitch);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyPitchModified get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_pitchModified, pitch);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifySpeedModified(int type, float speed) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_speedModified, speed);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifySpeedModified get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_speedModified, speed);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::notifyProgress(int type, float current, int total) {
    if (MAIN_THREAD == type) {
        jenv->CallVoidMethod(jobj, jmid_progress, current, total);
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyProgress get child jnienv wrong");
            return;
        }
        env->CallVoidMethod(jobj, jmid_progress, current, total);
        jvm->DetachCurrentThread();
    }
}

void
NotifyApplication::notifyRenderYUV(int type, int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv) {
    if (MAIN_THREAD == type) {
        //不支持从主线程传递YUV数据
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::notifyProgress get child jnienv wrong");
            return;
        }

        jbyteArray y = env->NewByteArray(width * height);
        env->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(fy));

        jbyteArray u = env->NewByteArray(width * height / 4);
        env->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(fu));

        jbyteArray v = env->NewByteArray(width * height / 4);
        env->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(fv));
        env->CallVoidMethod(jobj, jmid_renderyuv, width, height, y, u, v);

        env->DeleteLocalRef(y);
        env->DeleteLocalRef(u);
        env->DeleteLocalRef(v);
        jvm->DetachCurrentThread();
    }
}

bool NotifyApplication::callSupportVideo(int type, const char *ffCodeName) {
    bool support = false;
    if (MAIN_THREAD == type) {
        //[TODO]truyayog 不支持从主线程调用
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::callSupportVideo get child jnienv wrong");
            return support;
        }

        jstring type = env->NewStringUTF(ffCodeName);
        support = env->CallBooleanMethod(jobj, jmid_supportvideo, type);
        env->DeleteLocalRef(type);
        jvm->DetachCurrentThread();
    }
    return support;
}

void NotifyApplication::callInitMedaiCodec(int type, const char *mime, int width, int height,
                                           int csd0Size, int csd1Size, uint8_t *csd0,
                                           uint8_t *csd1) {
    if (MAIN_THREAD == type) {
        //不支持从主线程传递YUV数据
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::callInitMedaiCodec get child jnienv wrong");
            return;
        }
        jstring  mimeType = env->NewStringUTF(mime);
        jbyteArray  jCsd0 = env->NewByteArray(csd0Size);
        env->SetByteArrayRegion(jCsd0, 0 , csd0Size, reinterpret_cast<const jbyte *>(csd0));
        jbyteArray  jCsd1 = env->NewByteArray(csd1Size);
        env->SetByteArrayRegion(jCsd1, 0 , csd1Size, reinterpret_cast<const jbyte *>(csd1));

        env->CallVoidMethod(jobj, jmid_initmediacodec, mimeType, width, height, jCsd0, jCsd1);

        env->DeleteLocalRef(jCsd0);
        env->DeleteLocalRef(jCsd1);
        env->DeleteLocalRef(mimeType);
        jvm->DetachCurrentThread();
    }
}

void NotifyApplication::callHardDecodeAvPacket(int type, int size, uint8_t *data) {
    if (MAIN_THREAD == type) {
        //不支持从主线程传递YUV数据
    } else if (CHILD_THREAD == type) {
        JNIEnv* env;
        if (jvm->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("NotifyApplication::callHardDecodeAvPacket get child jnienv wrong");
            return;
        }

        jbyteArray jData = env->NewByteArray(size);
        env->SetByteArrayRegion(jData, 0, size, reinterpret_cast<const jbyte *>(data));
        env->CallVoidMethod(jobj, jmid_harddecodepacket, size, jData);
        env->DeleteLocalRef(jData);
        jvm->DetachCurrentThread();
    }
}
