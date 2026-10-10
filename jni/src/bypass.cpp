#include "../includes/types.h"
#include "../includes/offsets.h"
#include "../includes/memory.h"
#include "../includes/hook.h"
#include "../includes/bypass.h"
#include <android/log.h>
#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace bypass {

    typedef void* (*t_dlopen)(const char* filename, int flags);
    typedef void* (*t_android_dlopen_ext)(const char* filename, int flags, const void* extinfo);
    typedef int   (*t_sysprop)(const char* name, char* value);
    typedef void  (*t_abort)();
    typedef int   (*t_raise)(int sig);

    static t_dlopen orig_dlopen = nullptr;
    static t_android_dlopen_ext orig_android_dlopen_ext = nullptr;
    static t_sysprop orig_sysprop = nullptr;
    static t_abort orig_abort = nullptr;
    static t_raise orig_raise = nullptr;

    static volatile bool anogsPatched = false;
    static volatile bool anogsLoaded = false;
    static volatile bool watchdogRun = true;

    // ── Спуф device fingerprint ──
    struct PropSpoof { const char* name; const char* value; };
    static const PropSpoof propSpoofs[] = {
        { "ro.build.fingerprint", "google/walleye/walleye:8.1.0/OPM1.171019.011/4448085:user/release-keys" },
        { "ro.product.model", "Pixel 2" },
        { "ro.product.brand", "google" },
        { "ro.product.manufacturer", "Google" },
        { "ro.product.device", "walleye" },
        { "ro.product.name", "walleye" },
        { "ro.serialno", "FA7B21A0983" },
        { "ro.boot.serialno", "FA7B21A0983" },
    };
    static const int propSpoofCount = sizeof(propSpoofs) / sizeof(propSpoofs[0]);

    static int hooked_sysprop(const char* name, char* value) {
        if (name && value) {
            for (int i = 0; i < propSpoofCount; i++) {
                if (strcmp(name, propSpoofs[i].name) == 0) {
                    strcpy(value, propSpoofs[i].value);
                    return strlen(propSpoofs[i].value);
                }
            }
        }
        return orig_sysprop(name, value);
    }

    // ── Гвард: не даём anogs убить процесс через abort/raise ──
    // Вызов из main-потока пропускаем (легитимные крэши игры), из остальных — сон навечно.
    static bool isMainThread() {
        return gettid() == getpid();
    }

    static void hooked_abort() {
        if (!isMainThread()) {
            LOGI("[*] bypass: abort() blocked in side thread");
            while (true) sleep(3600);
        }
        if (orig_abort) orig_abort();
        while (true) sleep(3600);
    }

    static int hooked_raise(int sig) {
        if (!isMainThread() && (sig == SIGABRT || sig == SIGKILL || sig == SIGTERM)) {
            LOGI("[*] bypass: raise(%d) blocked in side thread", sig);
            while (true) sleep(3600);
        }
        return orig_raise(sig);
    }

    // ── Патч anogs ──
    static void patchAnogsNow() {
        if (anogsPatched) return;

        uintptr_t base = mem::getBase("libanogs.so");
        if (base == 0) return;

        LOGI("[*] bypass: patching libanogs at 0x%lx BEFORE its JNI_OnLoad", (unsigned long)base);

        mem::patch(base + ANOGS_1, "\x1F\x20\x03\xD5", 4);
        mem::patch(base + ANOGS_2, "\x00\x0A\x00\x35", 4);
        mem::patch(base + ANOGS_3, "\x00\x01\x00\x35", 4);
        mem::patch(base + ANOGS_4, "\x04\x00\x00\x14", 4);
        mem::patch(base + ANOGS_5, "\x50\x00\x00\x14", 4);

        anogsPatched = true;
        anogsLoaded = true;
        LOGI("[+] bypass: libanogs patched pre-init");
    }

    static void* hooked_dlopen(const char* filename, int flags) {
        void* handle = orig_dlopen(filename, flags);
        if (filename && strstr(filename, "libanogs.so")) {
            LOGI("[*] bypass: dlopen(\"%s\") intercepted", filename);
            patchAnogsNow();
        }
        return handle;
    }

    static void* hooked_android_dlopen_ext(const char* filename, int flags, const void* extinfo) {
        void* handle = orig_android_dlopen_ext(filename, flags, extinfo);
        if (filename && strstr(filename, "libanogs.so")) {
            LOGI("[*] bypass: android_dlopen_ext(\"%s\") intercepted", filename);
            patchAnogsNow();
        }
        return handle;
    }

    // ── Watchdog: anogs CRC-проверяет свой код и восстанавливает байты.
    //    Каждые 3 секунды сверяем и перепатчиваем. ──
    struct WatchTarget { uintptr_t addr; const char* bytes; size_t len; };
    static WatchTarget watchList[16];
    static int watchCount = 0;

    static void addToWatch(uintptr_t addr, const char* bytes, size_t len) {
        if (watchCount < 16) {
            watchList[watchCount].addr = addr;
            watchList[watchCount].bytes = bytes;
            watchList[watchCount].len = len;
            watchCount++;
        }
    }

    static void* watchdogThread(void* arg) {
        const char NOP4[4] = "\x1F\x20\x03\xD5";

        while (watchdogRun) {
            sleep(3);

            uintptr_t anogs = mem::getBase("libanogs.so");
            uintptr_t ue4 = mem::getBase("libUE4.so");

            if (anogs != 0) {
                // Перепатчиваем все 5 anogs-патчей, если игра их откатила
                const char* p2 = "\x00\x0A\x00\x35";
                const char* p3 = "\x00\x01\x00\x35";
                const char* p4 = "\x04\x00\x00\x14";
                const char* p5 = "\x50\x00\x00\x14";

                if (memcmp((void*)(anogs + ANOGS_1), NOP4, 4) != 0)
                    mem::patch(anogs + ANOGS_1, NOP4, 4);
                if (memcmp((void*)(anogs + ANOGS_2), p2, 4) != 0)
                    mem::patch(anogs + ANOGS_2, p2, 4);
                if (memcmp((void*)(anogs + ANOGS_3), p3, 4) != 0)
                    mem::patch(anogs + ANOGS_3, p3, 4);
                if (memcmp((void*)(anogs + ANOGS_4), p4, 4) != 0)
                    mem::patch(anogs + ANOGS_4, p4, 4);
                if (memcmp((void*)(anogs + ANOGS_5), p5, 4) != 0)
                    mem::patch(anogs + ANOGS_5, p5, 4);
            }

            if (ue4 != 0) {
                // Watchdog терминационных фикс-патчей
                if (memcmp((void*)(ue4 + OFF_Termination_Fix_1), NOP4, 4) != 0)
                    mem::patch(ue4 + OFF_Termination_Fix_1, NOP4, 4);
                if (memcmp((void*)(ue4 + OFF_Termination_Fix_2), NOP4, 4) != 0)
                    mem::patch(ue4 + OFF_Termination_Fix_2, NOP4, 4);
                if (memcmp((void*)(ue4 + OFF_Aimbot), "\x00\x00\xC8\x42", 4) != 0)
                    mem::write<float>(ue4 + OFF_Aimbot, 100.0f);
            }
        }
        return nullptr;
    }

    void initEarly() {
        LOGI("[*] bypass: installing early hooks");

        // 1. dlopen-хуки — патч anogs до его инициализации
        void* dlopen_sym = dlsym(RTLD_DEFAULT, "dlopen");
        void* dlext_sym = dlsym(RTLD_DEFAULT, "android_dlopen_ext");

        if (dlopen_sym) {
            hook::InlineHook::install(dlopen_sym, (void*)&hooked_dlopen, (void**)&orig_dlopen);
        }
        if (dlext_sym) {
            hook::InlineHook::install(dlext_sym, (void*)&hooked_android_dlopen_ext,
                                      (void**)&orig_android_dlopen_ext);
        }

        // 2. Спуф device fingerprint
        void* sysprop_sym = dlsym(RTLD_DEFAULT, "__system_property_get");
        if (sysprop_sym) {
            hook::InlineHook::install(sysprop_sym, (void*)&hooked_sysprop, (void**)&orig_sysprop);
            LOGI("[+] bypass: device fingerprint spoof active");
        }

        // 3. Гвард abort/raise
        void* abort_sym = dlsym(RTLD_DEFAULT, "abort");
        void* raise_sym = dlsym(RTLD_DEFAULT, "raise");
        if (abort_sym) {
            hook::InlineHook::install(abort_sym, (void*)&hooked_abort, (void**)&orig_abort);
        }
        if (raise_sym) {
            hook::InlineHook::install(raise_sym, (void*)&hooked_raise, (void**)&orig_raise);
        }

        // 4. Watchdog-поток
        pthread_t wd;
        pthread_create(&wd, nullptr, watchdogThread, nullptr);
        pthread_detach(wd);

        LOGI("[+] bypass: early hooks installed (dlopen, sysprop, abort/raise, watchdog)");
    }

    bool isAnogsReady() {
        return anogsLoaded;
    }

} // namespace bypass
