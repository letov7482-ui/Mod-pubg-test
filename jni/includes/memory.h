#pragma once

#include "types.h"
#include <string>
#include <dlfcn.h>

namespace mem {

    // Get module base address in self-process
    uintptr_t getBase(const char* libName);

    // Read memory (direct, we're in-process)
    template<typename T>
    T read(uintptr_t addr) {
        if (!addr) return T{};
        return *reinterpret_cast<T*>(addr);
    }

    template<typename T>
    void write(uintptr_t addr, T value) {
        if (!addr) return;
        *reinterpret_cast<T*>(addr) = value;
    }

    // Patch bytes (make writable first)
    bool patch(uintptr_t addr, const char* bytes, size_t len);

    // NOP instruction (ARM64: 0x1F 0x20 0x03 0xD5)
    bool nop(uintptr_t addr);

    // Make memory page writable+executable
    bool setProt(uintptr_t addr, size_t len, int prot);

    // Patch string
    void patchString(uintptr_t addr, const char* str, size_t maxLen);

    // Apply all game patches
    void applyPatches();

    // Apply anti-cheat bypass
    void bypassAnogs();

    // Restore original
    bool restore(uintptr_t addr, const char* original, size_t len);

} // namespace mem
