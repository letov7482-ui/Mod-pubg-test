#pragma once

#include "types.h"

namespace hook {

    // AArch64 inline hook implementation
    class InlineHook {
    public:
        static bool install(void* target, void* detour, void** original);
        static bool uninstall(void* target);

    private:
        static void makeTrampoline(void* target, void* detour, void** original, size_t overwriteLen);
        static size_t getInstructionLen(uintptr_t addr);
        static void fixBranches(void* trampoline, uintptr_t target, size_t len);
    };

    // Utility hook functions
    bool init();
    void* getOriginal(void* target);

    // Game-specific hooks
    void hookPostRender(void* target);
    void hookProcessEvent(void* target);

} // namespace hook
