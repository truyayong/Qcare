package com.example.playlib.videoRender;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.Surface;

import com.example.playlib.R;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by qiuyayong on 2019/2/12.
 * Email:qiuyayong@bigo.sg
 */

public class VideoRender implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
    private static final String TAG = "VideoRender";

    public static final int RENDER_YUV = 1;
    public static final int RENDER_MEDIACODEC = 2;

    private Context mContext;

    private final float[] vertexData = {
            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f
    };

    private final float[] textureData = {
            0f, 1f,
            1f, 1f,
            0f, 0f,
            1f, 0f
    };
    private FloatBuffer vertexBuffer;
    private FloatBuffer textureBuffer;
    private int renderType = RENDER_YUV;
    public void setRenderType(int type) {
        this.renderType = type;
    }

    //yuv
    private int programYuv;
    private int avPositionYuv;
    private int afPositionYuv;

    private int ySampler;
    private int uSampler;
    private int vSampler;
    private int[] textureIdYuv;

    private int widthYuv;
    private int heightYuv;
    private ByteBuffer y;
    private ByteBuffer u;
    private ByteBuffer v;

    //mediacodec
    private int programMediacodec;
    private int avPositionMediacodec;
    private int afPositionMediacodec;
    private int samplerOESMediacodec;
    private int textureIdMediacodec;
    private SurfaceTexture mSurfaceTexture;
    private Surface mSurface;
    private OnSurfaceCreateListener mOnSurfaceCreateListener;
    public void setOnSurfaceCreateListener (OnSurfaceCreateListener listener) {
        this.mOnSurfaceCreateListener = listener;
    }
    private OnRenderListener mOnRenderListener;
    public void setOnRenderListener(OnRenderListener listener) {
        this.mOnRenderListener = listener;
    }

    public VideoRender(Context context) {
        this.mContext = context;
        vertexBuffer = ByteBuffer.allocateDirect(vertexData.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(vertexData);
        vertexBuffer.position(0);
        textureBuffer = ByteBuffer.allocateDirect(textureData.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(textureData);
        textureBuffer.position(0);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        initRenderYuv();
        initRendermediaCodec();
    }

    private void initRenderYuv() {
        String vertexSource = ShaderUtil.readRawTxt(mContext, R.raw.vertex_shader);
        String fragmentSource = ShaderUtil.readRawTxt(mContext, R.raw.fragment_yuv);
        programYuv =ShaderUtil.createProgram(vertexSource, fragmentSource);
        if (programYuv == 0) {
            Log.e(TAG, "initRenderYuv fail : programYuv == 0");
            return;
        }

        avPositionYuv = GLES20.glGetAttribLocation(programYuv, "av_Position");
        afPositionYuv = GLES20.glGetAttribLocation(programYuv, "af_Position");

        ySampler = GLES20.glGetUniformLocation(programYuv, "sampler_y");
        uSampler = GLES20.glGetUniformLocation(programYuv, "sampler_u");
        vSampler = GLES20.glGetUniformLocation(programYuv, "sampler_v");

        textureIdYuv = new int[3];
        GLES20.glGenTextures(3, textureIdYuv, 0);

        for (int i = 0; i < 3; i++) {
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureIdYuv[i]);

            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        }
    }

    private void renderYUV() {
        if (widthYuv > 0 && heightYuv > 0
                && y != null && u != null && v != null) {
            GLES20.glUseProgram(programYuv);

            GLES20.glEnableVertexAttribArray(avPositionYuv);
            GLES20.glVertexAttribPointer(avPositionYuv, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer);

            GLES20.glEnableVertexAttribArray(afPositionYuv);
            GLES20.glVertexAttribPointer(afPositionYuv, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureIdYuv[0]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, widthYuv
                    , heightYuv, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, y);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureIdYuv[1]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, widthYuv / 2
                    , heightYuv / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, u);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureIdYuv[2]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, widthYuv / 2
                    , heightYuv / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, v);

            GLES20.glUniform1i(ySampler, 0);
            GLES20.glUniform1i(uSampler, 1);
            GLES20.glUniform1i(vSampler, 2);

            y.clear();
            u.clear();
            v.clear();
            y = null;
            u = null;
            v = null;
        }
    }

    public void refreshYuvRenderData(int width, int height, byte[] y, byte[] u, byte[] v) {
        this.widthYuv = width;
        this.heightYuv = height;
        this.y = ByteBuffer.wrap(y);
        this.u = ByteBuffer.wrap(u);
        this.v = ByteBuffer.wrap(v);
    }

    private void initRendermediaCodec() {
        String vertexSource = ShaderUtil.readRawTxt(mContext, R.raw.vertex_shader);
        String fragmentSource = ShaderUtil.readRawTxt(mContext, R.raw.fragment_mediacodec);
        programMediacodec = ShaderUtil.createProgram(vertexSource, fragmentSource);

        avPositionMediacodec = GLES20.glGetAttribLocation(programMediacodec, "av_Position");
        afPositionMediacodec = GLES20.glGetAttribLocation(programMediacodec, "af_Position");
        samplerOESMediacodec = GLES20.glGetUniformLocation(programMediacodec, "sTexture");

        int[] textureIds = new int[1];
        GLES20.glGenTextures(1, textureIds, 0);
        textureIdMediacodec = textureIds[0];

        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

        mSurfaceTexture = new SurfaceTexture(textureIdMediacodec);
        mSurface = new Surface(mSurfaceTexture);
        mSurfaceTexture.setOnFrameAvailableListener(this);
        if (mOnSurfaceCreateListener != null) {
            mOnSurfaceCreateListener.onSurfaceCreate(mSurface);
        }
    }

    private void renderMediaCodec() {
        mSurfaceTexture.updateTexImage();
        GLES20.glUseProgram(programMediacodec);

        GLES20.glEnableVertexAttribArray(avPositionMediacodec);
        GLES20.glVertexAttribPointer(avPositionMediacodec, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer);

        GLES20.glEnableVertexAttribArray(afPositionMediacodec);
        GLES20.glVertexAttribPointer(afPositionMediacodec, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureIdMediacodec);
        GLES20.glUniform1i(samplerOESMediacodec, 0);

    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        if (mOnRenderListener != null) {
            mOnRenderListener.onRender();
        }
    }

    public interface OnSurfaceCreateListener {
        void onSurfaceCreate(Surface surface);
    }

    public interface OnRenderListener {
        void onRender();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        if (renderType == RENDER_YUV) {
            renderYUV();
        } else if (renderType == RENDER_MEDIACODEC) {
            renderMediaCodec();
        }
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    }

}
