package com.example.playlib;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import com.example.playlib.videoRender.VideoGLSurfaceView;
import com.example.playlib.videoRender.VideoSupportUtil;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Created by Administrator on 2018/12/13 0013.
 */

public class PlayJniProxy {
    private static final String TAG = "PlayJniProxy";

    static {
        System.loadLibrary("PlayNative");
        System.loadLibrary("avutil-55");
        System.loadLibrary("swresample-2");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avformat-57");
        System.loadLibrary("swscale-4");
        System.loadLibrary("postproc-54");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avdevice-57");
    }

    public static final int PLAY_CHANNEL_RIGHT = 0;
    public static final int PLAY_CHANNEL_LEFT = 1;
    public static final int PLAY_CHANNEL_STEREO = 2;

    private String mSource = "";

    private int mVolume = 85;
    private int mChannelLayout = PLAY_CHANNEL_STEREO;

    public void setPlayProgressCallBack(PlayProgressCallBack callBack) {
        this.mPlayProgressCallBack = callBack;
    }

    private PlayProgressCallBack mPlayProgressCallBack;
    private Surface mSurface;
    private MediaFormat mMediaFormat;
    private MediaCodec mMediaCodec;
    private MediaCodec.BufferInfo mBufferinfo;

    /************************************************************************************************************************
     *暴露给java层的api
     *
     */

    public void setSurface(Surface surface) {
        this.mSurface = surface;
    }
    public void prepare() {
        prepare(mSource, mVolume,mChannelLayout);
    }

    /**
     * 播放器准备 ffmpeg初始化等操作
     * @param source
     * @param volume
     * @param layout 声道布局
     */
    public void prepare(String source, int volume,int layout) {
        mSource = source;
        mVolume = volume;
        mChannelLayout = layout;
        native_prepare(mSource, mVolume,mChannelLayout);
    }

    /**
     * 播放器开始播放 ffmpeg开始解码
     */
    public void start() {
        native_start();
    }

    public void next(String source) {
        mSource = source;
        stop();
    }

    public void resume() {
        native_resume();
    }

    public void pause() {
        native_pause();
    }

    public void stop() {
        native_stop();
    }

    public void seek(int progress) {
        native_seek(progress);
    }

    public int duration() {
        return native_duration();
    }

    public void setVolume(int percent) {
        if (percent < 0 || percent > 100) {
            return;
        }
        native_volume(percent);
    }

    public void switchChannel(int channel) {
        native_channel_switch(channel);
    }

    public void setPitch(float pitch) {
        native_pitch(pitch);
    }

    public void setSpeed(float speed) {
        native_speed(speed);
    }

    public int getSamplerate(){
        return native_samplerate();
    }

    /************************************************************************************************************************
     * native方法
     */
    private native void native_prepare(String source, int volume, int layout);
    private native void native_start();
    private native void native_resume();
    private native void native_pause();

    private native void native_stop();
    private native void native_seek(int progress);
    private native int native_duration();
    private native void native_volume(int percent);
    private native void native_channel_switch(int channel);
    private native void native_pitch(float pitch);
    private native void native_speed(float speed);
    private native int native_samplerate();

    /************************************************************************************************************************
     * native层通知应用层接口
     */

    /**
     * 播放器准备完成
     */
    private void onPrepared() {
        Log.i(TAG, "qmusic onPrepared");
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onPrepared();
        }
    }

    /**
     * 播放器开始完成
     */
    private void onStarted() {
        Log.i(TAG, "qmusic onStarted");
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onStarted();
        }
    }

    /**
     * 播放器继续播放完成
     */
    private void onResumed() {
        Log.i(TAG, "qmusic onResumed");
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onResumed();
        }
    }

    /**
     * 播放器暂停完成
     */
    private void onPaused() {
        Log.i(TAG, "qmusic onPaused");
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onPaused();
        }
    }

    /**
     * 播放器停止完成
     */
    private void onStopped() {
        Log.i(TAG, "qmusic onStopped");
        releaseMediaCodec();
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onStopped();
        }
    }

    private void onSeeked(int progress) {
        Log.i(TAG, "qmusic onSeeked progress : " + progress);
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onSeeked(progress);
        }
    }

    private void onVolumeModified(int percent) {
        Log.i(TAG, "qmusic onVolumeModified percent : " + percent);
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onVolumeModified(percent);
        }
    }

    private void onChannelLayoutModify(int layout) {
        Log.i(TAG, "qmusic onChannelLayoutModify layout : " + layout);
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onChannelLayoutModify(layout);
        }
    }

    private void onPitchModified(float pitch) {
        Log.i(TAG, "qmusic onPitchModified pitch : " + pitch);
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onPitchModified(pitch);
        }
    }

    private void onSpeedModified(float speed) {
        Log.i(TAG, "qmusic onSpeedModified speed : " + speed);
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onSpeedModified(speed);
        }
    }

    /**
     * 播放进度回调
     * @param currentProgress
     * @param total
     */
    private void onPlayProgress(float currentProgress, int total) {
//        Log.i(TAG, "qmusic onPlayProgress currentProgress : " + currentProgress + " Total : " + total);
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onPlayProgress(currentProgress, total);
        }
    }

    /**
     * 错误回调
     * @param code
     * @param msg
     */
    private void onError(int code, String msg) {
        Log.i(TAG, "qmusic onError code : " + code + " msg : " + msg);
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onError(code, msg);
        }
    }

    private void onRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (mPlayProgressCallBack != null) {
            mPlayProgressCallBack.onRenderYUV(width, height, y, u, v);
        }
    }

    public boolean onSupportMediaCodec(String ffCodeName) {
        return VideoSupportUtil.isSupportCodec(ffCodeName);
    }

    /**
     * 初始化mediaCodec
     * @param codecName
     * @param width
     * @param height
     * @param csd0
     * @param csd1
     */
    public void onInitMediaCodec(String codecName, int width, int height, byte[] csd0, byte[] csd1) {
        if (mSurface != null) {
            try {
                String mime = VideoSupportUtil.findVideoCodecName(codecName);
                mMediaFormat = MediaFormat.createVideoFormat(mime, width, height);
                mMediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
                mMediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd0));
                mMediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd1));
                Log.e(TAG, "mediaFormat : " + mMediaFormat.toString());
                mMediaCodec = MediaCodec.createDecoderByType(mime);

                mBufferinfo = new MediaCodec.BufferInfo();
                mMediaCodec.configure(mMediaFormat, mSurface, null, 0);
                mMediaCodec.start();
                if (mPlayProgressCallBack != null) {
                    mPlayProgressCallBack.onInitMediaCodec();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else {
            Log.e(TAG, "mSurface is null");
        }
    }

    public void hardDecodeAvPacket(int size, byte[] data) {
        Log.d(TAG, "hardDecodeAvPacket size : " + size + " datasize : " + data.length);
        if (mSurface != null && size > 0 && data != null) {
            try {
                int inputBufferIndex = mMediaCodec.dequeueInputBuffer(10);
                if (inputBufferIndex >= 0) {
                    ByteBuffer byteBuffer = mMediaCodec.getInputBuffers()[inputBufferIndex];
                    byteBuffer.clear();
                    byteBuffer.put(data);
                    mMediaCodec.queueInputBuffer(inputBufferIndex, 0, size, 0, 0);
                }
                int outputBufferIndex = mMediaCodec.dequeueOutputBuffer(mBufferinfo, 10);
                while (outputBufferIndex >= 0) {
                    mMediaCodec.releaseOutputBuffer(outputBufferIndex, true);
                    outputBufferIndex = mMediaCodec.dequeueOutputBuffer(mBufferinfo, 10);
                }
            } catch (Exception e) {
                Log.e(TAG, "hardDecodeAvPacket : ", e);
            }

        }
    }

    private void releaseMediaCodec() {
        if (mMediaCodec != null) {
            try {
                mMediaCodec.flush();
                mMediaCodec.stop();
                mMediaCodec.release();
            } catch (Exception e) {
                Log.e(TAG, " releaseMediaCodec : ", e);
            }


            mMediaCodec = null;
            mMediaFormat = null;
            mBufferinfo = null;
        }
    }

    /************************************************************************************************************************
     * 播放流程回调
     */
    public interface PlayProgressCallBack {
        void onPrepared();
        void onStarted();
        void onResumed();
        void onPaused();
        void onStopped();
        void onSeeked(int progress);
        void onVolumeModified(int percent);
        void onChannelLayoutModify(int layout);
        void onPitchModified(float pitch);
        void onSpeedModified(float speed);
        void onError(int code, String msg);
        void onPlayProgress(float currentProgress, int Total);
        void onRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v);
        void onInitMediaCodec();
    }
}
