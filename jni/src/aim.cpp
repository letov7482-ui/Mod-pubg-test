#include "../includes/aim.h"
#include "../includes/sdk.h"
#include "../includes/offsets.h"
#include "../includes/memory.h"
#include "../includes/hook.h"
#include <android/log.h>
#include <cmath>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

namespace aim {

    Config config = {
        .enabled = true,
        .silent = true,
        .smooth = true,
        .smoothing = 3.0f,
        .fov = 60.0f,
        .targetBone = BONE_HEAD,
        .visibleOnly = true,
        .autoFire = false,
        .predictionFactor = 1.0f
    };

    static void* currentTarget = nullptr;

    void init() {
        LOGI("[Aimbot] Initialized");
        hook::init();
    }

    // ── Aim assist enhancement (built-in game feature) ──
    void applyAimbotPatch(uintptr_t base) {
        if (base == 0) return;

        // The Aimbot_Offset likely contains a value that controls the built-in
        // aim assist strength. By setting it to maximum, we get a subtle but
        // effective aim enhancement that looks natural.

        // Method 1: Direct memory write to aim assist parameter
        // This makes the game's built-in auto-aim much stronger
        // but doesn't snap the camera (no visible aim snapping)

        // Write float value 100.0f (max aim assist)
        float aimAssistValue = 100.0f;
        mem::write(base + OFF_Aimbot, aimAssistValue);

        LOGI("[Aimbot] Aim assist enhanced at 0x%lx", base + OFF_Aimbot);
    }

    // ── Silent aim: redirect bullet direction ──
    void applySilentAim(uintptr_t base) {
        if (base == 0 || !config.silent) return;

        // Hook ShootBulletInner to redirect bullet to target's head
        void* shootBulletInner = reinterpret_cast<void*>(base + OFF_ShootBulletInner);
        if (!shootBulletInner) return;

        bool result = hook::install(
            shootBulletInner,
            reinterpret_cast<void*>(&hookedShootBulletInner),
            reinterpret_cast<void**>(&origShootBulletInner)
        );

        if (result) {
            LOGI("[Aimbot] Silent aim hook installed on ShootBulletInner");
        } else {
            LOGE("[-] Failed to install silent aim hook");
        }
    }

    // ── Find best target based on FOV and visibility ──
    void* findBestTarget() {
        void* localPawn = sdk::getLocalPawn();
        void* playerController = sdk::getPlayerController();
        void* cameraManager = sdk::getCameraManager();

        if (!localPawn || !playerController || !cameraManager) return nullptr;

        FRotator cameraRot = sdk::GetCameraRotation(cameraManager);
        FVector cameraLoc = sdk::GetCameraLocation(cameraManager);

        float bestScore = 999999.0f;
        void* bestActor = nullptr;

        u32 actorCount = 0;
        void** actors = (void**)sdk::getActors(actorCount);
        if (!actors) return nullptr;

        for (u32 i = 0; i < actorCount; i++) {
            void* actor = actors[i];
            if (!actor || actor == localPawn) continue;
            if (!sdk::isPlayer(actor)) continue;
            if (!sdk::isAlive(actor)) continue;
            if (sdk::isTeammate(actor)) continue;

            if (config.visibleOnly && !sdk::isVisible(localPawn, actor)) continue;

            // Get target bone position
            void* mesh = *(void**)((uintptr_t)actor + OFF_Actor_Mesh);
            if (!mesh) continue;

            FVector targetPos = sdk::getBonePosition(mesh, config.targetBone);
            FVector2D screenPos;

            if (!sdk::ProjectWorldLocationToScreen(playerController, targetPos, screenPos, false))
                continue;

            // Calculate angular distance from crosshair
            FVector toTarget = (targetPos - cameraLoc).Normalize();
            FVector camForward;
            float pitchRad = cameraRot.Pitch * M_PI / 180.0f;
            float yawRad = cameraRot.Yaw * M_PI / 180.0f;
            camForward = {
                cosf(pitchRad) * cosf(yawRad),
                cosf(pitchRad) * sinf(yawRad),
                sinf(pitchRad)
            };

            float angle = acosf(fmaxf(-1.0f, fminf(1.0f, toTarget.Dot(camForward)))) * 180.0f / M_PI;

            if (angle > config.fov) continue;

            // Score: prefer closest to crosshair and closest distance
            float dist = sdk::getDistanceTo(localPawn, actor) / 100.0f;
            float score = angle + dist * 0.1f;

            if (score < bestScore) {
                bestScore = score;
                bestActor = actor;
            }
        }

        return bestActor;
    }

    // ── Main aimbot processing (called every frame from render hook) ──
    void process() {
        if (!config.enabled) return;

        currentTarget = findBestTarget();
        // Actual aiming is handled by:
        // 1. Enhanced aim assist (memory patch)
        // 2. Silent aim (bullet redirect in ShootBulletInner hook)
        // 3. Smooth camera adjustment (optional, more detectable)
    }

    // ── Hooked ShootBulletInner for silent aim ──
    void hookedShootBulletInner(
        void* weapon,
        FVector& shootDir,
        FVector& shootLoc,
        float damage,
        float headshotMul,
        void* fireInst,
        void* hitResult,
        bool isBurst)
    {
        // Call original by default
        // origShootBulletInner(weapon, shootDir, shootLoc, damage, headshotMul, fireInst, hitResult, isBurst);

        if (!config.enabled || !config.silent || !currentTarget) {
            // Call original
            if (origShootBulletInner) {
                origShootBulletInner(weapon, shootDir, shootLoc, damage, headshotMul, fireInst, hitResult, isBurst);
            }
            return;
        }

        // Get target head position
        void* mesh = *(void**)((uintptr_t)currentTarget + OFF_Actor_Mesh);
        if (mesh) {
            FVector targetPos = sdk::getBonePosition(mesh, BONE_HEAD);

            // Add small random offset to look natural (human-like imperfection)
            float randX = (rand() % 200 - 100) / 10000.0f; // ±1cm
            float randY = (rand() % 200 - 100) / 10000.0f;
            float randZ = (rand() % 200 - 100) / 10000.0f;
            targetPos.X += randX;
            targetPos.Y += randY;
            targetPos.Z += randZ;

            // Redirect bullet direction
            FVector newDir = (targetPos - shootLoc).Normalize();
            shootDir = newDir;
        }

        // Call original with modified direction
        if (origShootBulletInner) {
            origShootBulletInner(weapon, shootDir, shootLoc, damage, headshotMul, fireInst, hitResult, isBurst);
        }
    }

} // namespace aim
