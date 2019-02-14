package com.example.playlib.videoRender;

import android.media.MediaCodecList;

import java.util.HashMap;
import java.util.Map;

/**
 * Created by qiuyayong on 2019/2/14.
 * Email:qiuyayong@bigo.sg
 */

public class VideoSupportUtil {

    private static Map<String, String> codecMap = new HashMap<>();
    static {
        codecMap.put("h264", "video/avc");
    }

    public static String findVideoCodecName(String ffCodecName) {
        if (codecMap.containsKey(ffCodecName)) {
            return codecMap.get(ffCodecName);
        }
        return "";
    }

    public static boolean isSupportCodec(String ffCodeName) {
        boolean supportVideo = false;
        int count = MediaCodecList.getCodecCount();
        for (int i = 0; i < count; i++) {
            String[] types = MediaCodecList.getCodecInfoAt(i).getSupportedTypes();
            for (String type : types) {
                if (type.equals(findVideoCodecName(ffCodeName))) {
                    supportVideo = true;
                    break;
                }
            }
            if (supportVideo) {
                break;
            }
        }
        return supportVideo;
    }
}
