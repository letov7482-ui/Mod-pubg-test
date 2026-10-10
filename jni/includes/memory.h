#pragma once

#include "types.h"
#include <string>
#include <dlfcn.h>

namespace mem {

    uintptr_t getBase(const char* libName);

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

    bool patch(uintptr_t addr, const char* bytes, size_t len);
    bool nop(uintptr_t addr);
    bool setProt(uintptr_t addr, size_t len, int prot);
    void patchString(uintptr_t addr, const char* str, size_t maxLen);
    void applyPatches();
    void bypassAnogs();
    bool restore(uintptr_t addr, const char* original, size_t len);

} // namespace mem
