#include "../includes/memory.h"
#include "../includes/offsets.h"
#include <android/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace mem {

    uintptr_t getBase(const char* libName) {
        std::ifstream maps("/proc/self/maps");
        std::string line;

        while (std::getline(maps, line)) {
            if (strstr(line.c_str(), libName) && strstr(line.c_str(), "r-xp")) {
                uintptr_t base = 0;
                sscanf(line.c_str(), "%lx-", &base);
                if (base != 0) {
                    return base;
                }
            }
        }
        return 0;
    }

    bool setProt(uintptr_t addr, size_t len, int prot) {
        uintptr_t pageStart = addr & ~(getpagesize() - 1);
        size_t pageLen = ((addr + len - 1) & ~(getpagesize() - 1)) - pageStart + getpagesize();
        return mprotect(reinterpret_cast<void*>(pageStart), pageLen, prot) == 0;
    }

    bool patch(uintptr_t addr, const char* bytes, size_t len) {
        if (!addr || !bytes || len == 0) return false;

        if (!setProt(addr, len, PROT_READ | PROT_WRITE | PROT_EXEC)) {
            LOGE("[-] mprotect failed at 0x%lx", (unsigned long)addr);
            return false;
        }

        memcpy(reinterpret_cast<void*>(addr), bytes, len);

        setProt(addr, len, PROT_READ | PROT_EXEC);

        __builtin___clear_cache(reinterpret_cast<char*>(addr), reinterpret_cast<char*>(addr + len));

        return true;
    }

    bool nop(uintptr_t addr) {
        // AArch64 NOP
        return patch(addr, "\x1F\x20\x03\xD5", 4);
    }

    void patchString(uintptr_t addr, const char* str, size_t maxLen) {
        setProt(addr, maxLen, PROT_READ | PROT_WRITE);
        strncpy(reinterpret_cast<char*>(addr), str, maxLen);
        setProt(addr, maxLen, PROT_READ);
    }

    bool restore(uintptr_t addr, const char* original, size_t len) {
        return patch(addr, original, len);
    }

    void bypassAnogs() {
        uintptr_t base = getBase("libanogs.so");
        if (base == 0) {
            LOGE("[-] libanogs.so not found for bypass");
            return;
        }

        LOGI("[*] Bypassing libanogs.so...");

        patch(base + ANOGS_1, "\x1F\x20\x03\xD5", 4);
        patch(base + ANOGS_2, "\x00\x0A\x00\x35", 4);
        patch(base + ANOGS_3, "\x00\x01\x00\x35", 4);
        patch(base + ANOGS_4, "\x04\x00\x00\x14", 4);
        patch(base + ANOGS_5, "\x50\x00\x00\x14", 4);

        LOGI("[+] libanogs.so patched: 5 patches applied");
    }

    void applyPatches() {
        uintptr_t base = getBase("libUE4.so");
        if (base == 0) return;

        nop(base + OFF_Termination_Fix_1);
        nop(base + OFF_Termination_Fix_2);
        nop(base + OFF_KillMessage);
        patch(base + OFF_FakeDamage_Fix, "\x1F\x20\x03\xD5", 4);
    }

} // namespace mem
