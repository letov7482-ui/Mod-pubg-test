#include "../includes/aim.h"
#include "../includes/sdk.h"
#include "../includes/offsets.h"
#include "../includes/memory.h"
#include "../includes/hook.h"
#include <android/log.h>
#include <cmath>
#include <cstdlib>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

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

    void* currentTarget = nullptr;
    tShootBulletInner origShootBulletInner = nullptr;

    void init() {
        LOGI("[Aimbot] Initialized");
        hook::init();
    }

    // ── Усиление встроенного aim assist ──
    void applyAimbotPatch(uintptr_t base) {
        if (base == 0) return;

        float aimAssistValue = 100.0f;
        mem::write<float>(base + OFF_Aimbot, aimAssistValue);

        LOGI("[Aimbot] Aim assist enhanced at 0x%lx", (unsigned long)(base + OFF_Aimbot));
    }

    // ── Silent aim: перенаправление пули ──
    void applySilentAim(uintptr_t base) {
        if (base == 0 || !config.silent) return;

        void* shootBulletInner = reinterpret_cast<void*>(base + OFF_ShootBulletInner);
        if (!shootBulletInner) return;

        bool result = hook::InlineHook::install(
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

    // ── Поиск лучшей цели ──
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
        void** actors = sdk::getActors(actorCount);
        if (!actors) return nullptr;

        float pitchRad = cameraRot.Pitch * M_PI / 180.0f;
        float yawRad = cameraRot.Yaw * M_PI / 180.0f;
        FVector camForward = {
            cosf(pitchRad) * cosf(yawRad),
            cosf(pitchRad) * sinf(yawRad),
            sinf(pitchRad)
        };

        for (u32 i = 0; i < actorCount; i++) {
            void* actor = actors[i];
            if (!actor || actor == localPawn) continue;
            if (!sdk::isPlayer(actor)) continue;
            if (!sdk::isAlive(actor)) continue;
            if (sdk::isTeammate(actor)) continue;

            if (config.visibleOnly && !sdk::isVisible(localPawn, actor)) continue;

            void* mesh = *(void**)((uintptr_t)actor + OFF_Actor_Mesh);
            if (!mesh) continue;

            FVector targetPos = sdk::getBonePosition(mesh, config.targetBone);
            if (targetPos.Size() < 0.01f) continue;

            FVector toTarget = (targetPos - cameraLoc).Normalize();
            float dot = fmaxf(-1.0f, fminf(1.0f, toTarget.Dot(camForward)));
            float angle = acosf(dot) * 180.0f / M_PI;

            if (angle > config.fov) continue;

            // Дистанция: через указатель на игру или по координатам
            float dist = 0.0f;
            if (sdk::GetDistanceTo) {
                dist = sdk::GetDistanceTo(localPawn, actor) / 100.0f;
            } else {
                void* rootA = *(void**)((uintptr_t)localPawn + OFF_Actor_RootComponent);
                void* rootB = *(void**)((uintptr_t)actor + OFF_Actor_RootComponent);
                if (rootA && rootB) {
                    FVector posB = *(FVector*)((uintptr_t)rootB + OFF_RootComp_Location);
                    FVector posA = *(FVector*)((uintptr_t)rootA + OFF_RootComp_Location);
                    dist = (posB - posA).Size() / 100.0f;
                }
            }

            float score = angle + dist * 0.1f;

            if (score < bestScore) {
                bestScore = score;
                bestActor = actor;
            }
        }

        return bestActor;
    }

    void process() {
        if (!config.enabled) {
            currentTarget = nullptr;
            return;
        }
        currentTarget = findBestTarget();
    }

    // ── Hooked ShootBulletInner ──
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
        if (!config.enabled || !config.silent || !currentTarget) {
            if (origShootBulletInner) {
                origShootBulletInner(weapon, shootDir, shootLoc, damage, headshotMul, fireInst, hitResult, isBurst);
            }
            return;
        }

        void* mesh = *(void**)((uintptr_t)currentTarget + OFF_Actor_Mesh);
        if (mesh) {
            FVector targetPos = sdk::getBonePosition(mesh, config.targetBone);

            if (targetPos.Size() > 0.01f) {
                // Рандом ±1см — имитация человеческой погрешности
                float randX = (rand() % 200 - 100) / 10000.0f;
                float randY = (rand() % 200 - 100) / 10000.0f;
                float randZ = (rand() % 200 - 100) / 10000.0f;
                targetPos.X += randX;
                targetPos.Y += randY;
                targetPos.Z += randZ;

                FVector newDir = (targetPos - shootLoc).Normalize();
                shootDir = newDir;
            }
        }

        if (origShootBulletInner) {
            origShootBulletInner(weapon, shootDir, shootLoc, damage, headshotMul, fireInst, hitResult, isBurst);
        }
    }

} // namespace aim
