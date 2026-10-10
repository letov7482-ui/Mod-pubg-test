#include "../includes/hook.h"
#include <android/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <map>
#include <cstring>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static std::map<void*, void*> g_originals;

namespace hook {

    static constexpr size_t HOOK_SIZE = 16;

    // ── InlineHook: статические методы ──

    size_t InlineHook::getInstructionLen(uintptr_t addr) {
        // AArch64 — все инструкции по 4 байта
        return 4;
    }

    void InlineHook::makeTrampoline(void* target, void* detour, void** original, size_t overwriteLen) {
        size_t tramSize = overwriteLen + 16;
        void* trampoline = mmap(nullptr, tramSize,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (trampoline == MAP_FAILED) {
            LOGE("[-] Failed to allocate trampoline");
            return;
        }

        // Копируем оригинальные инструкции
        memcpy(trampoline, target, overwriteLen);

        // Прыжок обратно: LDR X17, #8; BR X17; .quad targetNext
        uintptr_t branchAddr = reinterpret_cast<uintptr_t>(trampoline) + overwriteLen;
        uintptr_t targetNext = reinterpret_cast<uintptr_t>(target) + overwriteLen;

        *(u32*)(branchAddr) = 0x58000051;     // LDR X17, [PC, #8]
        *(u32*)(branchAddr + 4) = 0xD61F0220; // BR X17
        *(uintptr_t*)(branchAddr + 8) = targetNext;

        if (original) {
            void* origCopy = mmap(nullptr, overwriteLen,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            memcpy(origCopy, target, overwriteLen);
            *original = origCopy;
        }

        mprotect(trampoline, tramSize, PROT_READ | PROT_EXEC);
        __builtin___clear_cache(reinterpret_cast<char*>(trampoline),
                                reinterpret_cast<char*>(trampoline) + tramSize);
    }

    bool InlineHook::install(void* target, void* detour, void** original) {
        if (!target || !detour) return false;

        LOGI("[Hook] Installing hook at %p -> %p", target, detour);

        makeTrampoline(target, detour, original, HOOK_SIZE);

        long pagesize = sysconf(_SC_PAGESIZE);
        uintptr_t pageStart = reinterpret_cast<uintptr_t>(target) & ~(pagesize - 1);

        if (mprotect(reinterpret_cast<void*>(pageStart), pagesize,
                     PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            LOGE("[-] mprotect failed for hook");
            return false;
        }

        // Хук: LDR X16, #8; BR X16; .quad detour
        uintptr_t hookAddr = reinterpret_cast<uintptr_t>(target);
        *(u32*)(hookAddr) = 0x58000050;     // LDR X16, [PC, #8]
        *(u32*)(hookAddr + 4) = 0xD61F0200; // BR X16
        *(uintptr_t*)(hookAddr + 8) = reinterpret_cast<uintptr_t>(detour);

        mprotect(reinterpret_cast<void*>(pageStart), pagesize, PROT_READ | PROT_EXEC);
        __builtin___clear_cache(reinterpret_cast<char*>(target),
                                reinterpret_cast<char*>(target) + HOOK_SIZE);

        if (original && *original) {
            g_originals[target] = *original;
        }

        LOGI("[+] Hook installed at %p", target);
        return true;
    }

    bool InlineHook::uninstall(void* target) {
        auto it = g_originals.find(target);
        if (it == g_originals.end()) return false;

        long pagesize = sysconf(_SC_PAGESIZE);
        uintptr_t pageStart = reinterpret_cast<uintptr_t>(target) & ~(pagesize - 1);
        mprotect(reinterpret_cast<void*>(pageStart), pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);

        memcpy(target, it->second, HOOK_SIZE);

        mprotect(reinterpret_cast<void*>(pageStart), pagesize, PROT_READ | PROT_EXEC);
        __builtin___clear_cache(reinterpret_cast<char*>(target),
                                reinterpret_cast<char*>(target) + HOOK_SIZE);

        g_originals.erase(it);
        return true;
    }

    // ── Свободные функции ──

    bool init() {
        LOGI("[*] Hook system initialized");
        return true;
    }

    void* getOriginal(void* target) {
        auto it = g_originals.find(target);
        return (it != g_originals.end()) ? it->second : nullptr;
    }

    void hookPostRender(void* target) {
    }

    void hookProcessEvent(void* target) {
    }

} // namespace hook
