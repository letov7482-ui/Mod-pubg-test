#include "../includes/types.h"
#include "../includes/offsets.h"
#include "../includes/memory.h"
#include "../includes/hook.h"
#include "../includes/bypass.h"
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
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

static uintptr_t ue4Base = 0;
static bool initialized = false;

void doWork();
void applyMiscPatches();

static void* waitForLibraries(void* arg) {
    LOGI("[*] Background thread started");

    // 1. dlopen/sysprop/abort-хуки — СРАЗУ, до загрузки anogs
    bypass::initEarly();

    // 2. Ждём libUE4.so
    int attempts = 0;
    while (attempts < 300) {
        ue4Base = mem::getBase("libUE4.so");
        if (ue4Base != 0) {
            LOGI("[+] libUE4.so found at: 0x%lx", (unsigned long)ue4Base);
            break;
        }
        usleep(200000);
        attempts++;
    }
    if (ue4Base == 0) {
        LOGE("[-] libUE4.so not found");
        return nullptr;
    }

    // 3. Ждём пока dlopen-хук запатчит anogs до его JNI_OnLoad
    attempts = 0;
    while (attempts < 300 && !bypass::isAnogsReady()) {
        usleep(200000);
        attempts++;
    }
    if (bypass::isAnogsReady()) {
        LOGI("[+] anogs pre-init patched, safe to proceed");
    } else {
        LOGW("[!] anogs not intercepted in time — fallback direct patch");
        mem::bypassAnogs();
    }

    // 4. Патчи UE4 — немедленно, без sleep
    doWork();
    initialized = true;
    LOGI("[+] All patches applied. Mod is active!");
    return nullptr;
}

void doWork() {
    LOGI("[*] Applying patches...");

    esp::init();
    LOGI("[+] ESP initialized");

    aim::init();
    aim::applyAimbotPatch(ue4Base);
    aim::applySilentAim(ue4Base);
    LOGI("[+] Aimbot initialized");

    sdk::init(ue4Base);
    LOGI("[+] SDK initialized");

    applyMiscPatches();
    LOGI("[+] Misc patches applied");

    LOGI("[*] All done. Mod is fully operational.");
}

void applyMiscPatches() {
    mem::patch(ue4Base + OFF_Unlock_120fps, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Unlock_Hdr, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Ipad_View, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_No_Grass_Tree, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_SmallCross, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Recoil_Small, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Ping_Fix, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Flash_Speed, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Flash_Speed2, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Fix_Stuck, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Termination_Fix_1, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_Termination_Fix_2, "\x1F\x20\x03\xD5", 4);
    mem::patch(ue4Base + OFF_6Hr_TimeFix, "\x1F\x20\x03\xD5", 4);
}

extern "C" {

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_init(JNIEnv* env, jobject thiz) {
        LOGI("[*] Native library loaded");
        pthread_t thread;
        pthread_create(&thread, nullptr, waitForLibraries, nullptr);
        pthread_detach(thread);
    }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPEnabled(JNIEnv* env, jobject thiz, jboolean v) { esp::config.enabled = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPBox(JNIEnv* env, jobject thiz, jboolean v) { esp::config.box = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPSkeleton(JNIEnv* env, jobject thiz, jboolean v) { esp::config.skeleton = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPHealthBar(JNIEnv* env, jobject thiz, jboolean v) { esp::config.healthBar = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPName(JNIEnv* env, jobject thiz, jboolean v) { esp::config.name = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPDistance(JNIEnv* env, jobject thiz, jboolean v) { esp::config.distance = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPLine(JNIEnv* env, jobject thiz, jboolean v) { esp::config.line = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setESPMaxDistance(JNIEnv* env, jobject thiz, jfloat v) { esp::config.maxDistance = v; }

    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotEnabled(JNIEnv* env, jobject thiz, jboolean v) { aim::config.enabled = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotSilent(JNIEnv* env, jobject thiz, jboolean v) { aim::config.silent = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotVisibleOnly(JNIEnv* env, jobject thiz, jboolean v) { aim::config.visibleOnly = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotFOV(JNIEnv* env, jobject thiz, jfloat v) { aim::config.fov = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotSmoothing(JNIEnv* env, jobject thiz, jfloat v) { aim::config.smoothing = v; }
    JNIEXPORT void JNICALL
    Java_com_pubg_mod_NativeBridge_setAimbotBone(JNIEnv* env, jobject thiz, jint v) { aim::config.targetBone = v; }

} // extern "C"
