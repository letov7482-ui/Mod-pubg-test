#include "../includes/types.h"
#include "../includes/offsets.h"
#include "../includes/memory.h"
#include "../includes/hook.h"
#include "../includes/sdk.h"
#include "../includes/esp.h"
#include "../includes/aim.h"

#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static uintptr_t ue4Base = 0;
static uintptr_t anogsBase = 0;
static bool initialized = false;

// ── Wait for libUE4.so to load ──
static void* waitForLibraries(void* arg) {
    LOGI("[*] Background thread started, waiting for libraries...");

    int attempts = 0;
    while (attempts < 300) { // 5 min timeout
        ue4Base = mem::getBase("libUE4.so");
        if (ue4Base != 0) {
            LOGI("[+] libUE4.so found at: 0x%lx", ue4Base);

            anogsBase = mem::getBase("libanogs.so");
            if (anogsBase != 0) {
                LOGI("[+] libanogs.so found at: 0x%lx", anogsBase);
            }

            // Wait for game to fully initialize
            sleep(8);

            // Apply all patches
            doWork();

            initialized = true;
            LOGI("[+] All patches applied. Mod is active!");
            break;
        }
        usleep(1000000); // 1 second
        attempts++;
    }

    if (!initialized) {
        LOGE("[-] Failed to find libUE4.so after 300 attempts");
    }

    return nullptr;
}

// ── Main work function ──
void doWork() {
    LOGI("[*] Applying patches...");

    // 1. Bypass anti-cheat first
    if (anogsBase != 0) {
        mem::bypassAnogs();
        LOGI("[+] Anti-cheat bypassed");
    }

    // 2. Apply ESP patches
    esp::init();
    LOGI("[+] ESP initialized");

    // 3. Apply Aimbot patches
    aim::init();
    aim::applyAimbotPatch(ue4Base);
    aim::applySilentAim(ue4Base);
    LOGI("[+] Aimbot initialized");

    // 4. Initialize SDK
    sdk::init(ue4Base);
    LOGI("[+] SDK initialized");

    // 5. Apply misc patches
    applyMiscPatches();
    LOGI("[+] Misc patches applied");

    LOGI("[*] All done. Mod is fully operational.");
}

// ── Misc patches (120fps, HDR, etc.) ──
void applyMiscPatches() {
    // Unlock 120 FPS
    mem::patch(ue4Base + OFF_Unlock_120fps, "\x01\x00\xa0\xe1\x1e\xff\x2f\xe1", 8);

    // Unlock HDR
    mem::patch(ue4Base + OFF_Unlock_Hdr, "\x01\x00\xa0\xe1\x1e\xff\x2f\xe1", 8);

    // iPad View
    mem::patch(ue4Base + OFF_Ipad_View, "\x1F\x20\x03\xD5", 4);

    // No Grass/Tree
    mem::patch(ue4Base + OFF_No_Grass_Tree, "\x1F\x20\x03\xD5", 4);

    // Small Crosshair
    mem::patch(ue4Base + OFF_SmallCross, "\x1F\x20\x03\xD5", 4);

    // Reduced Recoil
    mem::patch(ue4Base + OFF_Recoil_Small, "\x1F\x20\x03\xD5", 4);

    // Ping Fix
    mem::patch(ue4Base + OFF_Ping_Fix, "\x1F\x20\x03\xD5", 4);

    // Flash Speed Fix
    mem::patch(ue4Base + OFF_Flash_Speed, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Flash_Speed2, "\x1F\x20\x03\xD5", 4);

    // Fix Stuck
    mem::patch(ue4Base + OFF_Fix_Stuck, "\x1F\x20\x03\xD5", 4);

    // Termination Fix
    mem::patch(ue4Base + OFF_Termination_Fix_1, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Termination_Fix_2, "\x1F\x20\x03\xD5", 4);

    // 6 Hour Fix
    mem::patch(ue4Base + OFF_6Hr_TimeFix, "\x1F\x20\x03\xD5", 4);
}

// ── JNI entry point ──
extern "C" {

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_init(JNIEnv* env, jobject thiz) {
        LOGI("[*] Native library loaded");

        pthread_t thread;
        pthread_create(&thread, nullptr, waitForLibraries, nullptr);
        pthread_detach(thread);
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPEnabled(JNIEnv* env, jobject thiz, jboolean enabled) {
        esp::config.enabled = enabled;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotEnabled(JNIEnv* env, jobject thiz, jboolean enabled) {
        aim::config.enabled = enabled;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotFOV(JNIEnv* env, jobject thiz, jfloat fov) {
        aim::config.fov = fov;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotSmoothing(JNIEnv* env, jobject thiz, jfloat s) {
        aim::config.smoothing = s;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotBone(JNIEnv* env, jobject thiz, jint bone) {
        aim::config.targetBone = bone;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotSilent(JNIEnv* env, jobject thiz, jboolean silent) {
        aim::config.silent = silent;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotVisibleOnly(JNIEnv* env, jobject thiz, jboolean v) {
        aim::config.visibleOnly = v;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPBox(JNIEnv* env, jobject thiz, jboolean v) {
        esp::config.box = v;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPSkeleton(JNIEnv* env, jobject thiz, jboolean v) {
        esp::config.skeleton = v;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPHealthBar(JNIEnv* env, jobject thiz, jboolean v) {
        esp::config.healthBar = v;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPName(JNIEnv* env, jobject thiz, jboolean v) {
        esp::config.name = v;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPDistance(JNIEnv* env, jobject thiz, jboolean v) {
        esp::config.distance = v;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPLine(JNIEnv* env, jobject thiz, jboolean v) {
        esp::config.line = v;
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPMaxDistance(JNIEnv* env, jobject thiz, jfloat v) {
        esp::config.maxDistance = v;
    }

} // extern "C"
