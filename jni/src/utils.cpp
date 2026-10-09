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

    // ── Resolved function pointers ──
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

        // Resolve function pointers from offsets
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

        // Navigate: GWorld -> Levels -> Actors -> PlayerController -> LocalPlayer
        // This depends on the specific UE4 version and PUBG Mobile's structure
        // Simplified: read from known offset
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
        // Canvas is obtained during PostRender callback
        // We'll cache it from the hook
        static void* canvas = nullptr;
        return canvas;
    }

    void* getLevel() {
        void* gWorld = getGWorld();
        if (!gWorld) return nullptr;
        return *(void**)((uintptr_t)gWorld + 0x0038); // PersistentLevel
    }

    void* getActors(u32& count) {
        void* level = getLevel();
        if (!level) { count = 0; return nullptr; }

        // TArray<AActor*>
        void** actors = *(void***)((uintptr_t)level + 0x00A0); // Actors array
        count = *(u32*)((uintptr_t)level + 0x00A8); // Array count

        return actors;
    }

    bool isPlayer(void* actor) {
        if (!actor) return false;
        void* actorClass = *(void**)((uintptr_t)actor + OFF_ObjectClass);
        if (!actorClass) return false;

        // Check class name contains "PlayerPawn" or similar
        // We need to read the class FName
        void* className = *(void**)((uintptr_t)actorClass + OFF_ObjectName);
        if (!className) return false;

        // Compare FName index to known player class
        // This is simplified - in practice we'd have a proper name lookup
        // For PUBG Mobile, check if it's a character with a mesh
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

        // GetBonePosWithRotation is typically a static function
        // that takes mesh and bone index
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
        // Read player name from PlayerState
        // Offset depends on PUBG Mobile version
        return (const wchar_t*)(*(uintptr_t*)((uintptr_t)playerState + 0x03A0));
    }

} // namespace sdk
