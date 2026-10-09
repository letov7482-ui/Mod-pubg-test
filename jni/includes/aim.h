#pragma once

#include "types.h"
#include "sdk.h"

namespace aim {

    struct Config {
        bool enabled;
        bool silent;
        bool smooth;
        float smoothing;
        float fov;
        int targetBone;
        bool visibleOnly;
        bool autoFire;
        float predictionFactor;
    };

    extern Config config;
    extern void* currentTarget;

    void init();
    void process();
    void* findBestTarget();

    void applyAimbotPatch(uintptr_t base);
    void applySilentAim(uintptr_t base);

    typedef void (*tShootBulletInner)(void* weapon, FVector& shootDir, FVector& shootLoc, float damage, float headshotMul, void* fireInst, void* hitResult, bool isBurst);
    extern tShootBulletInner origShootBulletInner;

    void hookedShootBulletInner(void* weapon, FVector& shootDir, FVector& shootLoc, float damage, float headshotMul, void* fireInst, void* hitResult, bool isBurst);

} // namespace aim
