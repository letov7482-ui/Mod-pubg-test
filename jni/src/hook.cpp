#include "../includes/hook.h"
#include <android/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <map>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// Cache: target address -> trampoline
static std::map<void*, void*> g_originals;

namespace hook {

    // AArch64 inline hook using trampoline approach
    // Overwrites target with branch to detour
    // Trampoline contains original instructions + branch back

    static constexpr size_t HOOK_SIZE = 16; // 4 instructions for AArch64

    size_t getInstructionLen(uintptr_t addr) {
        // For AArch64 all instructions are 4 bytes
        // Check for literal pools or data
        // Simple: return 4
        return 4;
    }

    void makeTrampoline(void* target, void* detour, void** original, size_t overwriteLen) {
        // Allocate trampoline (RWX)
        size_t tramSize = overwriteLen + 16; // original bytes + branch back
        void* trampoline = mmap(nullptr, tramSize,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (trampoline == MAP_FAILED) {
            LOGE("[-] Failed to allocate trampoline");
            return;
        }

        // Copy original instructions to trampoline
        memcpy(trampoline, target, overwriteLen);

        // Add branch back to (target + overwriteLen)
        uintptr_t branchAddr = reinterpret_cast<uintptr_t>(trampoline) + overwriteLen;
        uintptr_t targetNext = reinterpret_cast<uintptr_t>(target) + overwriteLen;

        // LDR X17, #8; BR X17; .quad targetNext
        *(u32*)(branchAddr) = 0x58000051; // LDR X17, [PC, #8]
        *(u32*)(branchAddr + 4) = 0xD61F0220; // BR X17
        *(uintptr_t*)(branchAddr + 8) = targetNext;

        // Copy target to original for restore
        if (original) {
            void* origCopy = mmap(nullptr, overwriteLen,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            memcpy(origCopy, target, overwriteLen);
            *original = origCopy;
        }

        // Make trampoline executable
        mprotect(trampoline, tramSize, PROT_READ | PROT_EXEC);
        __builtin___clear_cache(reinterpret_cast<char*>(trampoline),
                                reinterpret_cast<char*>(trampoline) + tramSize);
    }

    bool install(void* target, void* detour, void** original) {
        if (!target || !detour) return false;

        LOGI("[Hook] Installing hook at %p -> %p", target, detour);

        // Create trampoline with original code
        makeTrampoline(target, detour, original, HOOK_SIZE);

        // Make target writable
        long pagesize = sysconf(_SC_PAGESIZE);
        uintptr_t pageStart = reinterpret_cast<uintptr_t>(target) & ~(pagesize - 1);

        if (mprotect(reinterpret_cast<void*>(pageStart), pagesize,
                     PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            LOGE("[-] mprotect failed for hook");
            return false;
        }

        // Write hook: LDR X16, #8; BR X16; .quad detour
        uintptr_t hookAddr = reinterpret_cast<uintptr_t>(target);
        *(u32*)(hookAddr) = 0x58000050; // LDR X16, [PC, #8]
        *(u32*)(hookAddr + 4) = 0xD61F0200; // BR X16
        *(uintptr_t*)(hookAddr + 8) = reinterpret_cast<uintptr_t>(detour);

        // Restore permissions and clear cache
        mprotect(reinterpret_cast<void*>(pageStart), pagesize, PROT_READ | PROT_EXEC);
        __builtin___clear_cache(reinterpret_cast<char*>(target),
                                reinterpret_cast<char*>(target) + HOOK_SIZE);

        g_originals[target] = original ? *original : nullptr;

        LOGI("[+] Hook installed at %p", target);
        return true;
    }

    bool uninstall(void* target) {
        auto it = g_originals.find(target);
        if (it == g_originals.end()) return false;

        // Restore original bytes
        // ... (restore from saved copy)
        g_originals.erase(it);
        return true;
    }

    void* getOriginal(void* target) {
        auto it = g_originals.find(target);
        return (it != g_originals.end()) ? it->second : nullptr;
    }

    bool init() {
        LOGI("[*] Hook system initialized");
        return true;
    }

    void hookPostRender(void* target) {
        // Will be called from esp.cpp when we have the right target
    }

    void hookProcessEvent(void* target) {
        // Will be called from sdk.cpp
    }

} // namespace hook
