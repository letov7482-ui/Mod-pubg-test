package com.pubg.mod;

import android.content.Context;

public class Injector {
    public static void load(Context ctx) {
        try {
            System.loadLibrary("pmod");
            NativeBridge.init();
            ModMenu.init(ctx);
        } catch (Throwable t) {
        }
    }
}
