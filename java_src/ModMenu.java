package com.pubg.mod;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class ModMenu {

    // ── Палитра ──
    private static final int C_BG       = 0xF00F1219; // панель
    private static final int C_HEADER   = 0xFF1A1F2B; // шапка
    private static final int C_ACCENT   = 0xFFFF2E4C; // неон-красный
    private static final int C_TEXT     = 0xFFF2F4F8;
    private static final int C_SUB      = 0xFF9AA3AD;
    private static final int C_ROW      = 0x141A2030; // строка
    private static final int C_ON       = 0xFF22C55E;
    private static final int C_OFF      = 0xFF3A4150;
    private static final int C_SLIDER   = 0xFF2E3542;

    private static Context appCtx;
    private static WindowManager wm;
    private static View bubble;
    private static View panel;
    private static WindowManager.LayoutParams bubbleParams;
    private static WindowManager.LayoutParams panelParams;
    private static boolean panelOpen = false;

    public static void init(Context ctx) {
        if (ctx == null) return;
        Context ac = ctx.getApplicationContext();
        if (!(ac instanceof Application)) return;
        appCtx = ac;

        ((Application) ac).registerActivityLifecycleCallbacks(new Application.ActivityLifecycleCallbacks() {
            @Override public void onActivityResumed(Activity a) { attach(a); }
            @Override public void onActivityPaused(Activity a) { detach(); }
            @Override public void onActivityCreated(Activity a, Bundle b) {}
            @Override public void onActivityStarted(Activity a) {}
            @Override public void onActivityStopped(Activity a) {}
            @Override public void onActivitySaveInstanceState(Activity a, Bundle b) {}
            @Override public void onActivityDestroyed(Activity a) {}
        });
    }

    private static int dp(float v) {
        return (int) (v * appCtx.getResources().getDisplayMetrics().density + 0.5f);
    }

    private static GradientDrawable round(int color, float radiusDp) {
        GradientDrawable d = new GradientDrawable();
        d.setColor(color);
        d.setCornerRadius(dp(radiusDp));
        return d;
    }

    // ── Прикрепление к окну активности ──
    private static void attach(Activity activity) {
        detach();
        try {
            wm = activity.getWindowManager();

            bubbleParams = new WindowManager.LayoutParams(
                    dp(46), dp(46),
                    WindowManager.LayoutParams.TYPE_APPLICATION,
                    WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                            | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                    android.graphics.PixelFormat.TRANSLUCENT);
            bubbleParams.token = activity.getWindow().getDecorView().getApplicationWindowToken();
            bubbleParams.gravity = Gravity.TOP | Gravity.LEFT;
            bubbleParams.x = dp(12);
            bubbleParams.y = dp(220);

            panelParams = new WindowManager.LayoutParams(
                    WindowManager.LayoutParams.WRAP_CONTENT,
                    WindowManager.LayoutParams.WRAP_CONTENT,
                    WindowManager.LayoutParams.TYPE_APPLICATION,
                    WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                            | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                    android.graphics.PixelFormat.TRANSLUCENT);
            panelParams.token = activity.getWindow().getDecorView().getApplicationWindowToken();
            panelParams.gravity = Gravity.TOP | Gravity.LEFT;
            panelParams.x = dp(12);
            panelParams.y = dp(80);

            bubble = buildBubble();
            wm.addView(bubble, bubbleParams);
        } catch (Throwable t) {
            wm = null;
        }
    }

    private static void detach() {
        try {
            if (wm != null && bubble != null) { wm.removeView(bubble); bubble = null; }
            if (wm != null && panel != null) { wm.removeView(panel); panel = null; }
            panelOpen = false;
        } catch (Throwable ignored) {}
    }

    // ── Плавающий шарик ──
    private static View buildBubble() {
        FrameLayout fl = new FrameLayout(appCtx);
        TextView tv = new TextView(appCtx);
        tv.setText("P");
        tv.setTextColor(Color.WHITE);
        tv.setTypeface(Typeface.create("monospace", Typeface.BOLD));
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        tv.setGravity(Gravity.CENTER);
        tv.setBackground(round(C_ACCENT, 23));
        fl.addView(tv, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        attachDrag(fl, bubbleParams, new Runnable() {
            @Override public void run() { togglePanel(); }
        });
        return fl;
    }

    private static void togglePanel() {
        try {
            if (panelOpen) {
                if (panel != null) wm.removeView(panel);
                panel = null;
                panelOpen = false;
            } else {
                panel = buildPanel();
                wm.addView(panel, panelParams);
                panelOpen = true;
            }
        } catch (Throwable ignored) {}
    }

    // ── Драг + клик ──
    private static void attachDrag(View v, final WindowManager.LayoutParams params, final Runnable onClick) {
        v.setOnTouchListener(new View.OnTouchListener() {
            float downRawX, downRawY, startX, startY;
            boolean moved;
            @Override public boolean onTouch(View view, MotionEvent e) {
                switch (e.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        downRawX = e.getRawX(); downRawY = e.getRawY();
                        startX = params.x; startY = params.y;
                        moved = false;
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        float dx = e.getRawX() - downRawX;
                        float dy = e.getRawY() - downRawY;
                        if (Math.abs(dx) > dp(6) || Math.abs(dy) > dp(6)) moved = true;
                        params.x = (int) (startX + dx);
                        params.y = (int) (startY + dy);
                        try { wm.updateViewLayout(view, params); } catch (Throwable ignored) {}
                        return true;
                    case MotionEvent.ACTION_UP:
                        if (!moved && onClick != null) onClick.run();
                        return true;
                }
                return false;
            }
        });
    }

    // ── Кастомный тоггл ──
    private static View toggleRow(String label, final boolean initial, final Toggler t) {
        LinearLayout row = new LinearLayout(appCtx);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dp(12), dp(9), dp(12), dp(9));
        GradientDrawable bg = round(C_ROW, 10);
        row.setBackground(bg);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(6);
        row.setLayoutParams(lp);

        TextView name = new TextView(appCtx);
        name.setText(label);
        name.setTextColor(C_TEXT);
        name.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        name.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
        LinearLayout.LayoutParams nlp = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
        row.addView(name, nlp);

        final TextView sw = new TextView(appCtx);
        sw.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        sw.setTypeface(Typeface.create("monospace", Typeface.BOLD));
        sw.setPadding(dp(14), dp(3), dp(14), dp(3));
        sw.setGravity(Gravity.CENTER);
        final boolean[] state = { initial };
        paintSwitch(sw, state[0]);

        sw.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                state[0] = !state[0];
                paintSwitch(sw, state[0]);
                t.toggle(state[0]);
            }
        });
        row.addView(sw, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        t.toggle(initial);
        return row;
    }

    private interface Toggler { void toggle(boolean on); }

    private static void paintSwitch(TextView sw, boolean on) {
        sw.setText(on ? "ON" : "OFF");
        sw.setTextColor(on ? Color.BLACK : C_SUB);
        sw.setBackground(round(on ? C_ON : C_OFF, 14));
    }

    // ── Слайдер ──
    private static View sliderRow(String label, int min, int max, int initial, final Slider cb) {
        LinearLayout box = new LinearLayout(appCtx);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(12), dp(9), dp(12), dp(9));
        box.setBackground(round(C_ROW, 10));
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(6);
        box.setLayoutParams(lp);

        final TextView name = new TextView(appCtx);
        name.setTextColor(C_TEXT);
        name.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        name.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
        box.addView(name);

        SeekBar sb = new SeekBar(appCtx);
        sb.setMax(max - min);
        sb.setProgress(initial - min);
        sb.getProgressDrawable().setColorFilter(C_ACCENT, android.graphics.PorterDuff.Mode.SRC_IN);
        sb.getThumb().setColorFilter(Color.WHITE, android.graphics.PorterDuff.Mode.SRC_IN);
        sb.setPadding(dp(8), dp(4), dp(8), dp(0));
        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean fromUser) {
                int val = s.getProgress() + min;
                name.setText(label + ": " + val);
                cb.changed(val);
            }
            @Override public void onStartTrackingTouch(SeekBar s) {}
            @Override public void onStopTrackingTouch(SeekBar s) {}
        });
        name.setText(label + ": " + initial);
        box.addView(sb);
        cb.changed(initial);
        return box;
    }

    private interface Slider { void changed(int v); }

    private static TextView sectionTitle(String text) {
        TextView t = new TextView(appCtx);
        t.setText(text);
        t.setTextColor(C_ACCENT);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        t.setTypeface(Typeface.create("monospace", Typeface.BOLD));
        t.setPadding(dp(4), dp(10), dp(4), dp(0));
        return t;
    }

    // ── Главная панель ──
    private static View buildPanel() {
        LinearLayout root = new LinearLayout(appCtx);
        root.setOrientation(LinearLayout.VERTICAL);
        GradientDrawable panelBg = round(C_BG, 16);
        panelBg.setStroke(dp(1), 0x33FF2E4C);
        root.setBackground(panelBg);
        int w = dp(280);
        root.setLayoutParams(new ViewGroup.LayoutParams(w, ViewGroup.LayoutParams.WRAP_CONTENT));

        // Шапка
        LinearLayout header = new LinearLayout(appCtx);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);
        header.setPadding(dp(14), dp(11), dp(14), dp(11));
        GradientDrawable hbg = new GradientDrawable(
                GradientDrawable.Orientation.TL_BR,
                new int[]{0xFF1E2430, C_HEADER});
        hbg.setCornerRadii(new float[]{dp(16), dp(16), dp(16), dp(16), 0, 0, 0, 0});
        header.setBackground(hbg);

        TextView title = new TextView(appCtx);
        title.setText("PMOD");
        title.setTextColor(C_ACCENT);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        title.setTypeface(Typeface.create("monospace", Typeface.BOLD));
        header.addView(title, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        TextView ver = new TextView(appCtx);
        ver.setText("v1.0 · 4.6.0");
        ver.setTextColor(C_SUB);
        ver.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        ver.setTypeface(Typeface.create("monospace", Typeface.NORMAL));
        header.addView(ver);

        LinearLayout body = new LinearLayout(appCtx);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(dp(10), dp(4), dp(10), dp(12));

        // ── ESP ──
        body.addView(sectionTitle("// ESP"));
        body.addView(toggleRow("Enable ESP", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setESPEnabled(on); }}));
        body.addView(toggleRow("Box", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setESPBox(on); }}));
        body.addView(toggleRow("Skeleton", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setESPSkeleton(on); }}));
        body.addView(toggleRow("Health Bar", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setESPHealthBar(on); }}));
        body.addView(toggleRow("Name", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setESPName(on); }}));
        body.addView(toggleRow("Distance", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setESPDistance(on); }}));
        body.addView(toggleRow("Snapline", false, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setESPLine(on); }}));
        body.addView(sliderRow("ESP Range", 50, 600, 300, new Slider() {
            public void changed(int v) { NativeBridge.setESPMaxDistance(v); }}));

        // ── AIMBOT ──
        body.addView(sectionTitle("// AIMBOT"));
        body.addView(toggleRow("Enable Aimbot", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setAimbotEnabled(on); }}));
        body.addView(toggleRow("Silent Aim", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setAimbotSilent(on); }}));
        body.addView(toggleRow("Visible Only", true, new Toggler() {
            public void toggle(boolean on) { NativeBridge.setAimbotVisibleOnly(on); }}));
        body.addView(sliderRow("FOV", 20, 180, 60, new Slider() {
            public void changed(int v) { NativeBridge.setAimbotFOV(v); }}));
        body.addView(sliderRow("Bone (6=Head 5=Neck 4=Chest)", 0, 6, 6, new Slider() {
            public void changed(int v) { NativeBridge.setAimbotBone(v); }}));

        // ── BYPASS ──
        body.addView(sectionTitle("// BYPASS"));
        LinearLayout st = new LinearLayout(appCtx);
        st.setOrientation(LinearLayout.HORIZONTAL);
        st.setGravity(Gravity.CENTER_VERTICAL);
        st.setPadding(dp(12), dp(8), dp(12), dp(8));
        st.setBackground(round(C_ROW, 10));
        LinearLayout.LayoutParams slp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        slp.topMargin = dp(6);
        st.setLayoutParams(slp);

        TextView dot = new TextView(appCtx);
        dot.setText("●");
        dot.setTextColor(C_ON);
        dot.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        st.addView(dot);

        TextView stx = new TextView(appCtx);
        stx.setText("  ANOGS GUARD: ACTIVE");
        stx.setTextColor(C_ON);
        stx.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        stx.setTypeface(Typeface.create("monospace", Typeface.BOLD));
        st.addView(stx);

        body.addView(st);

        // Футер
        TextView foot = new TextView(appCtx);
        foot.setText("watchdog · props spoof · dlopen hook");
        foot.setTextColor(C_SUB);
        foot.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        foot.setTypeface(Typeface.create("monospace", Typeface.NORMAL));
        foot.setGravity(Gravity.CENTER);
        foot.setPadding(0, dp(8), 0, dp(2));
        body.addView(foot);

        root.addView(header);
        root.addView(body);

        // Драг панели за шапку
        attachDrag(header, panelParams, null);
        return root;
    }
          }
