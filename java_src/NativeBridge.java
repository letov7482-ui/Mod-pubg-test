package com.pubg.mod;

public class NativeBridge {
    public static native void init();

    public static native void setESPEnabled(boolean v);
    public static native void setESPBox(boolean v);
    public static native void setESPSkeleton(boolean v);
    public static native void setESPHealthBar(boolean v);
    public static native void setESPName(boolean v);
    public static native void setESPDistance(boolean v);
    public static native void setESPLine(boolean v);
    public static native void setESPMaxDistance(float v);

    public static native void setAimbotEnabled(boolean v);
    public static native void setAimbotSilent(boolean v);
    public static native void setAimbotVisibleOnly(boolean v);
    public static native void setAimbotFOV(float v);
    public static native void setAimbotSmoothing(float v);
    public static native void setAimbotBone(int v);
}
