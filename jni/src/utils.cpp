#include "../includes/types.h"
#include "../includes/sdk.h"
#include "../includes/memory.h"
#include "../includes/esp.h"
#include "../includes/aim.h"
#include "../includes/offsets.h"
#include "../includes/hook.h"
#include <android/log.h>
#include <cmath>
#include <random>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

namespace sdk {

    // ── Function pointers ──
    tProjectWorldLocationToScreen ProjectWorldLocationToScreen = nullptr;
    tLineOfSightTo LineOfSightTo = nullptr;
    tGetBonePosWithRotation GetBonePos = nullptr;
    tK2_DrawLine DrawLine = nullptr;
    tK2_DrawText DrawText = nullptr;
    tGetCameraRotation GetCameraRotation = nullptr;
    tGetCameraLocation GetCameraLocation = nullptr;
    tGetDistanceTo GetDistanceTo = nullptr;

    static uintptr_t base = 0;

    bool init(uintptr_t ue4Base) {
        base = ue4Base;
        if (base == 0) return false;

        ProjectWorldLocationToScreen = (tProjectWorldLocationToScreen)(base + OFF_ProjectWorldToScreen);
        LineOfSightTo = (tLineOfSightTo)(base + OFF_LineOfSightTo);
        GetBonePos = (tGetBonePosWithRotation)(base + OFF_GetBonePos);
        DrawLine = (tK2_DrawLine)(base + OFF_K2_DrawLine);
        DrawText = (tK2_DrawText)(base + OFF_K2_DrawText);
        GetCameraRotation = (tGetCameraRotation)(base + OFF_GetCameraRotation);
        GetCameraLocation = (tGetCameraLocation)(base + OFF_GetCameraLocation);
        GetDistanceTo = (tGetDistanceTo)(base + OFF_GetDistanceTo);

        LOGI("[SDK] Function pointers resolved");
        return true;
    }

    void* getGWorld() {
        if (base == 0) return nullptr;
        return *(void**)(base + OFF_GWorld);
    }

    void* getLocalPlayer() {
        void* gWorld = getGWorld();
        if (!gWorld) return nullptr;
        return *(void**)(base + OFF_GWorld + OFF_Actor_LocalPlayers);
    }

    void* getLocalPawn() {
        void* pc = getPlayerController();
        if (!pc) return nullptr;
        return *(void**)((uintptr_t)pc + OFF_AcknowledgedPawn);
    }

    void* getPlayerController() {
        void* lp = getLocalPlayer();
        if (!lp) return nullptr;
        return *(void**)((uintptr_t)lp + OFF_PlayerController);
    }

    void* getCameraManager() {
        void* pc = getPlayerController();
        if (!pc) return nullptr;
        return *(void**)((uintptr_t)pc + OFF_PlayerCameraManager);
    }

    void* getCanvas() {
        static void* canvas = nullptr;
        return canvas;
    }

    void* getLevel() {
        void* gWorld = getGWorld();
        if (!gWorld) return nullptr;
        return *(void**)((uintptr_t)gWorld + 0x0038); // PersistentLevel
    }

    void** getActors(u32& count) {
        count = 0;
        void* level = getLevel();
        if (!level) return nullptr;

        void** actors = *(void***)((uintptr_t)level + 0x00A0);
        count = *(u32*)((uintptr_t)level + 0x00A8);

        return actors;
    }

    bool isPlayer(void* actor) {
        if (!actor) return false;
        void* mesh = *(void**)((uintptr_t)actor + OFF_Actor_Mesh);
        return mesh != nullptr;
    }

    bool isAlive(void* actor) {
        if (!actor) return false;
        u8 bDead = *(u8*)((uintptr_t)actor + OFF_Actor_bDead);
        float health = *(float*)((uintptr_t)actor + OFF_Actor_Health);
        return !bDead && health > 0.0f;
    }

    bool isTeammate(void* actor) {
        if (!actor) return false;
        void* localPawn = getLocalPawn();
        if (!localPawn) return false;

        int localTeam = *(int*)((uintptr_t)localPawn + OFF_Actor_TeamNum);
        int actorTeam = *(int*)((uintptr_t)actor + OFF_Actor_TeamNum);
        return localTeam == actorTeam;
    }

    bool isVisible(void* from, void* to) {
        if (!LineOfSightTo || !from || !to) return false;
        FVector viewPoint(0, 0, 0);
        return LineOfSightTo(from, to, viewPoint);
    }

    FVector getBonePosition(void* mesh, int boneIndex) {
        if (!mesh || !GetBonePos) return FVector();
        return GetBonePos(mesh, boneIndex);
    }

    float getHealth(void* actor) {
        if (!actor) return 0.0f;
        return *(float*)((uintptr_t)actor + OFF_Actor_Health);
    }

    int getTeamNumber(void* actor) {
        if (!actor) return -1;
        return *(int*)((uintptr_t)actor + OFF_Actor_TeamNum);
    }

    FVector getPosition(void* actor) {
        if (!actor) return FVector();
        void* rootComp = *(void**)((uintptr_t)actor + OFF_Actor_RootComponent);
        if (!rootComp) return FVector();
        return *(FVector*)((uintptr_t)rootComp + OFF_RootComp_Location);
    }

    const wchar_t* getPlayerName(void* playerState) {
        if (!playerState) return L"Unknown";
        return (const wchar_t*)(*(uintptr_t*)((uintptr_t)playerState + 0x03A0));
    }

} // namespace sdk
