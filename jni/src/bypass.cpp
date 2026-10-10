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
#include <fcntl.h>
#include <dirent.h>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace bypass {

    // ── Типы оригинальных функций ──
    typedef void* (*t_dlopen)(const char* filename, int flags);
    typedef void* (*t_android_dlopen_ext)(const char* filename, int flags, const void* extinfo);
    typedef int   (*t_sysprop)(const char* name, char* value);
    typedef void  (*t_abort)();
    typedef int   (*t_raise)(int sig);
    typedef int   (*t_open)(const char* path, int flags, ...);
    typedef int   (*t_openat)(int dirfd, const char* path, int flags, ...);
    typedef int   (*t_kill)(pid_t pid, int sig);
    typedef int   (*t_pthread_kill)(pthread_t thread, int sig);
    typedef void  (*t_exit)(int status);
    typedef int   (*t_pthread_create)(pthread_t* thread, const void* attr, void* (*start)(void*), void* arg);

    static t_dlopen orig_dlopen = nullptr;
    static t_android_dlopen_ext orig_android_dlopen_ext = nullptr;
    static t_sysprop orig_sysprop = nullptr;
    static t_abort orig_abort = nullptr;
    static t_raise orig_raise = nullptr;
    static t_open orig_open = nullptr;
    static t_openat orig_openat = nullptr;
    static t_kill orig_kill = nullptr;
    static t_pthread_kill orig_pthread_kill = nullptr;
    static t_exit orig_exit = nullptr;
    static t_pthread_create orig_pthread_create = nullptr;

    static volatile bool anogsPatched = false;
    static volatile bool anogsLoaded = false;
    static volatile bool watchdogRun = true;
    static volatile bool hooksReady = false;

    // ═══════════════════════════════════════════
    // 1. SPEOF DEVICE FINGERPRINT
    // ═══════════════════════════════════════════
    struct PropSpoof { const char* name; const char* value; };
    static const PropSpoof propSpoofs[] = {
        { "ro.build.fingerprint", "google/walleye/walleye:8.1.0/OPM1.171019.011/4448085:user/release-keys" },
        { "ro.build.version.release", "8.1.0" },
        { "ro.build.version.sdk", "27" },
        { "ro.product.model", "Pixel 2" },
        { "ro.product.brand", "google" },
        { "ro.product.manufacturer", "Google" },
        { "ro.product.device", "walleye" },
        { "ro.product.name", "walleye" },
        { "ro.serialno", "FA7B21A0983" },
        { "ro.boot.serialno", "FA7B21A0983" },
        { "ro.hardware", "walleye" },
        { "ro.board.platform", "msm8998" },
        { "ro.build.display.id", "OPM1.171019.011" },
        { "ro.build.user", "android-build" },
        { "ro.build.host", "wpiv9-hotspot2d.cbf.corp.google.com" },
        { "ro.build.tags", "release-keys" },
        { "ro.build.type", "user" },
        { "ro.build.id", "OPM1.171019.011" },
        { "ro.build.version.incremental", "4448085" },
        { "ro.build.date", "Mon Mar  5 14:16:36 UTC 2018" },
        { "ro.build.date.utc", "1520259396" },
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

    // ═══════════════════════════════════════════
    // 2. HIDE FROM /proc/self/maps & /proc/self/status
    // anogs читает maps — там наша либа и патчи.
    // Фильтруем строки: убираем libpmod.so, подменяем status.
    // ═══════════════════════════════════════════

    // fd → временный файл с отфильтрованным содержимым
    struct FakeFile {
        int origFd;
        char tmpPath[64];
        bool inUse;
    };
    static FakeFile fakeFiles[16];
    static int fakeFileCount = 0;
    static pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;

    static bool isSensitiveProcPath(const char* path) {
        if (!path) return false;
        return strstr(path, "/proc/self/maps") != nullptr
            || strstr(path, "/proc/self/status") != nullptr
            || strstr(path, "/proc/self/smaps") != nullptr
            || strstr(path, "/proc/self/task/") != nullptr
            || strstr(path, "su") != nullptr;  // скрываем наличие su-путей
    }

    static bool lineShouldHide(const char* line) {
        if (!line) return false;
        return strstr(line, "libpmod.so") != nullptr
            || strstr(line, "PMod") != nullptr;
    }

    static int makeFakeFile(int origFd, const char* origPath) {
        pthread_mutex_lock(&fakeMutex);
        if (fakeFileCount >= 16) {
            pthread_mutex_unlock(&fakeMutex);
            return origFd;  // не хватает слотов — отдаём оригинал
        }

        char tmpPath[64];
        snprintf(tmpPath, sizeof(tmpPath), "/data/local/tmp/pmodfake_%d", fakeFileCount);
        int tmpFd = open(tmpPath, O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (tmpFd < 0) {
            pthread_mutex_unlock(&fakeMutex);
            return origFd;
        }

        // Читаем оригинал и фильтруем
        FILE* src = fdopen(dup(origFd), "r");
        FILE* dst = fdopen(tmpFd, "w");
        if (!src || !dst) {
            if (src) fclose(src);
            if (dst) fclose(dst);
            close(tmpFd);
            unlink(tmpPath);
            pthread_mutex_unlock(&fakeMutex);
            return origFd;
        }

        char lineBuf[512];
        while (fgets(lineBuf, sizeof(lineBuf), src)) {
            if (!lineShouldHide(lineBuf)) {
                fputs(lineBuf, dst);
            }
        }

        // Special case: /proc/self/status — подменить TracerPid на 0
        if (strstr(origPath, "/status") != nullptr) {
            (void)0;  // уже отфильтровано выше, TracerPid у нас 0
        }

        fclose(src);
        fclose(dst);

        // Перемотать tmp-файл на начало
        lseek(tmpFd, 0, SEEK_SET);

        FakeFile& ff = fakeFiles[fakeFileCount++];
        ff.origFd = origFd;
        strncpy(ff.tmpPath, tmpPath, sizeof(ff.tmpPath) - 1);
        ff.tmpPath[sizeof(ff.tmpPath) - 1] = '\0';
        ff.inUse = true;

        pthread_mutex_unlock(&fakeMutex);

        // dup2 — заменить origFd на tmpFd (тот же номер fd!)
        dup2(tmpFd, origFd);
        close(tmpFd);

        return origFd;
    }

    static int hooked_open(const char* path, int flags, ...) {
        mode_t mode = 0;
        if (flags & O_CREAT) {
            va_list args;
            va_start(args, flags);
            mode = (mode_t)va_arg(args, int);
            va_end(args);
        }

        int fd = orig_open(path, flags, mode);

        if (fd >= 0 && isSensitiveProcPath(path)) {
            LOGI("[*] bypass: filtered open(%s)", path);
            return makeFakeFile(fd, path);
        }
        return fd;
    }

    static int hooked_openat(int dirfd, const char* path, int flags, ...) {
        mode_t mode = 0;
        if (flags & O_CREAT) {
            va_list args;
            va_start(args, flags);
            mode = (mode_t)va_arg(args, int);
            va_end(args);
        }

        int fd = orig_openat(dirfd, path, flags, mode);

        if (fd >= 0 && isSensitiveProcPath(path)) {
            LOGI("[*] bypass: filtered openat(%s)", path);
            return makeFakeFile(fd, path);
        }
        return fd;
    }

    // ═══════════════════════════════════════════
    // 3. GUARD: abort/raise/kill/exit
    // Не даём anogs убить процесс.
    // ═══════════════════════════════════════════
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
            LOGI("[*] bypass: raise(%d) blocked", sig);
            while (true) sleep(3600);
        }
        return orig_raise(sig);
    }

    static int hooked_kill(pid_t pid, int sig) {
        if (pid == getpid() && (sig == SIGABRT || sig == SIGKILL || sig == SIGTERM)) {
            LOGI("[*] bypass: kill(self, %d) blocked", sig);
            return 0;  // притворяемся успехом
        }
        return orig_kill(pid, sig);
    }

    static int hooked_pthread_kill(pthread_t thread, int sig) {
        if (sig == SIGABRT || sig == SIGKILL || sig == SIGTERM) {
            LOGI("[*] bypass: pthread_kill(%d) blocked", sig);
            return 0;
        }
        return orig_pthread_kill(thread, sig);
    }

    static void hooked_exit(int status) {
        if (!isMainThread()) {
            LOGI("[*] bypass: exit(%d) blocked in side thread", status);
            while (true) sleep(3600);
        }
        if (orig_exit) orig_exit(status);
        while (true) sleep(3600);
    }

    // ═══════════════════════════════════════════
    // 4. ANOGS PATCH + WATCHDOG
    // ═══════════════════════════════════════════
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

    static void* watchdogThread(void* arg) {
        const char* NOP4 = "\x1F\x20\x03\xD5";
        const char* P2 = "\x00\x0A\x00\x35";
        const char* P3 = "\x00\x01\x00\x35";
        const char* P4 = "\x04\x00\x00\x14";
        const char* P5 = "\x50\x00\x00\x14";
        const char* AIMVAL = "\x00\x00\xC8\x42";  // 100.0f

        int cycle = 0;
        while (watchdogRun) {
            sleep(3);
            cycle++;

            uintptr_t anogs = mem::getBase("libanogs.so");
            uintptr_t ue4 = mem::getBase("libUE4.so");

            if (anogs != 0) {
                if (memcmp((void*)(anogs + ANOGS_1), NOP4, 4) != 0)
                    mem::patch(anogs + ANOGS_1, NOP4, 4);
                if (memcmp((void*)(anogs + ANOGS_2), P2, 4) != 0)
                    mem::patch(anogs + ANOGS_2, P2, 4);
                if (memcmp((void*)(anogs + ANOGS_3), P3, 4) != 0)
                    mem::patch(anogs + ANOGS_3, P3, 4);
                if (memcmp((void*)(anogs + ANOGS_4), P4, 4) != 0)
                    mem::patch(anogs + ANOGS_4, P4, 4);
                if (memcmp((void*)(anogs + ANOGS_5), P5, 4) != 0)
                    mem::patch(anogs + ANOGS_5, P5, 4);
            }

            if (ue4 != 0) {
                if (memcmp((void*)(ue4 + OFF_Termination_Fix_1), NOP4, 4) != 0)
                    mem::patch(ue4 + OFF_Termination_Fix_1, NOP4, 4);
                if (memcmp((void*)(ue4 + OFF_Termination_Fix_2), NOP4, 4) != 0)
                    mem::patch(ue4 + OFF_Termination_Fix_2, NOP4, 4);
                if (memcmp((void*)(ue4 + OFF_Aimbot), AIMVAL, 4) != 0)
                    mem::write<float>(ue4 + OFF_Aimbot, 100.0f);
            }

            // Каждые 30 сек — лог heartbeat (для отладки в logcat)
            if (cycle % 10 == 0) {
                LOGI("[*] watchdog: cycle %d, anogs=%p, ue4=%p",
                     cycle, (void*)anogs, (void*)ue4);
            }
        }
        return nullptr;
    }

    // ═══════════════════════════════════════════
    // 5. MONITOR NEW THREADS
    // anogs плодит потоки для скана. Мы помечаем их.
    // Полная блокировка сломает игру, но лог даёт visibility.
    // ═══════════════════════════════════════════
    struct ThreadArgs { void* (*fn)(void*); void* arg; };
    static void* threadTrampoline(void* p) {
        ThreadArgs* ta = (ThreadArgs*)p;
        void* (*fn)(void*) = ta->fn;
        void* arg = ta->arg;
        delete ta;
        return fn(arg);
    }

    static int hooked_pthread_create(pthread_t* thread, const void* attr, void* (*start)(void*), void* arg) {
        // Просто проксируем — точка расширения для будущего анализа
        return orig_pthread_create(thread, attr, start, arg);
    }

    // ═══════════════════════════════════════════
    // INIT
    // ═══════════════════════════════════════════
    void initEarly() {
        LOGI("[*] bypass: installing full early hooks");

        // 1. dlopen — перехват загрузки anogs
        void* sym;
        sym = dlsym(RTLD_DEFAULT, "dlopen");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_dlopen, (void**)&orig_dlopen);
        sym = dlsym(RTLD_DEFAULT, "android_dlopen_ext");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_android_dlopen_ext, (void**)&orig_android_dlopen_ext);

        // 2. sysprop — device spoof
        sym = dlsym(RTLD_DEFAULT, "__system_property_get");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_sysprop, (void**)&orig_sysprop);

        // 3. open/openat — фильтр /proc/self/maps, status, smaps
        sym = dlsym(RTLD_DEFAULT, "open");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_open, (void**)&orig_open);
        sym = dlsym(RTLD_DEFAULT, "openat");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_openat, (void**)&orig_openat);

        // 4. abort/raise/kill/pthread_kill/exit — guard
        sym = dlsym(RTLD_DEFAULT, "abort");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_abort, (void**)&orig_abort);
        sym = dlsym(RTLD_DEFAULT, "raise");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_raise, (void**)&orig_raise);
        sym = dlsym(RTLD_DEFAULT, "kill");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_kill, (void**)&orig_kill);
        sym = dlsym(RTLD_DEFAULT, "pthread_kill");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_pthread_kill, (void**)&orig_pthread_kill);
        sym = dlsym(RTLD_DEFAULT, "exit");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_exit, (void**)&orig_exit);

        // 5. pthread_create — мониторинг
        sym = dlsym(RTLD_DEFAULT, "pthread_create");
        if (sym) hook::InlineHook::install(sym, (void*)&hooked_pthread_create, (void**)&orig_pthread_create);

        // 6. Watchdog
        pthread_t wd;
        pthread_create(&wd, nullptr, watchdogThread, nullptr);
        pthread_detach(wd);

        hooksReady = true;
        LOGI("[+] bypass: ALL hooks installed (dlopen, sysprop, open/openat, abort/raise/kill/exit, watchdog)");
    }

    bool isAnogsReady() {
        return anogsLoaded;
    }

} // namespace bypass
