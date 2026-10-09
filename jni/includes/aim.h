#pragma once

#include "types.h"
#include "sdk.h"

namespace aim {

    struct Config {
        bool enabled;
        bool silent;         // Silent aim (bullet redirect)
        bool smooth;         // Smooth aiming
        float smoothing;     // 0.1 = very smooth, 10.0 = instant
        float fov;           // Aim FOV in degrees
        int targetBone;      // 6=head, 5=neck, 4=chest
        bool visibleOnly;
        bool autoFire;
        float predictionFactor;
    };

    extern Config config;

    void init();
    void process();
    void* findBestTarget();

    // Apply patches for built-in aimbot assist enhancement
    void applyAimbotPatch(uintptr_t base);
    void applySilentAim(uintptr_t base);

    // Hook on ShootBulletInner for bullet redirection
    typedef void (*tShootBulletInner)(void* weapon, FVector& shootDir, FVector& shootLoc, float damage, float headshotMul, void* fireInst, void* hitResult, bool isBurst);
    static tShootBulletInner origShootBulletInner = nullptr;

    void hookedShootBulletInner(void* weapon, FVector& shootDir, FVector& shootLoc, float damage, float headshotMul, void* fireInst, void* hitResult, bool isBurst);

} // namespace aim
