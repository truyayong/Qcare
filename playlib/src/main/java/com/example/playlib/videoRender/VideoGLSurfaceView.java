package com.example.playlib.videoRender;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;

/**
 * Created by qiuyayong on 2019/2/12.
 * Email:qiuyayong@bigo.sg
 */

public class VideoGLSurfaceView extends GLSurfaceView {

    private static final String TAG = "VideoGLSurfaceView";

    private VideoRender mRender;
    public VideoGLSurfaceView(Context context) {
        this(context, null);
    }

    public VideoGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mRender = new VideoRender(context);
        setRenderer(mRender);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    public void refreshData(int width, int height, byte[] y, byte[] u, byte[] v) {
        Log.d(TAG, "refreshData width : " + width + " height : " + height);
        if (mRender != null) {
            mRender.refreshYuvRenderData(width, height, y, u, v);
            requestRender();
        }
    }
}
