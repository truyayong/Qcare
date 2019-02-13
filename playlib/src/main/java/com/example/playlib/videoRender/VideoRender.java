package com.example.playlib.videoRender;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

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

public class VideoRender implements GLSurfaceView.Renderer {
    private static final String TAG = "VideoRender";

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
    private int programYuv;
    private int avPositionYuv;
    private int afPositionYuv;
    private int textureid;

    private int ySampler;
    private int uSampler;
    private int vSampler;
    private int[] textureIdYuv;

    private int widthYuv;
    private int heightYuv;
    private ByteBuffer y;
    private ByteBuffer u;
    private ByteBuffer v;

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
    }

    private void initRenderYuv() {
        String vertexSource = ShaderUtil.readRawTxt(mContext, R.raw.vertex_shader);
        String fragmentSource = ShaderUtil.readRawTxt(mContext, R.raw.fragment_shader);
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

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        renderYUV();
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
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
}
