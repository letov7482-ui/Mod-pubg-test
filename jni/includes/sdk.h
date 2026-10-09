#pragma once

#include "types.h"

namespace sdk {

    // ── UE4 Core ──
    struct UObject {
        void* vtable;            // 0x0000
        u32    ObjectFlags;       // 0x0008
        u32    InternalIndex;     // 0x000C
        void*  ClassPrivate;      // 0x0010
        void*  NamePrivate;       // 0x0018
        void*  OuterPrivate;      // 0x0020
    };

    struct FName {
        u32 Index;
        u32 Number;
    };

    // ── Function pointer types ──
    typedef bool (*tProjectWorldLocationToScreen)(void* playerController, FVector& worldPos, FVector2D& screenPos, bool relative);
    typedef bool (*tLineOfSightTo)(void* actor, void* other, FVector viewPoint);
    typedef FVector (*tGetBonePosWithRotation)(void* mesh, int boneIndex);
    typedef void (*tK2_DrawLine)(void* canvas, FVector2D screenPosA, FVector2D screenPosB, float thickness, FLinearColor color);
    typedef void (*tK2_DrawText)(void* canvas, void* font, const wchar_t* text, FVector2D screenPos, FLinearColor color, float kerning, FVector2D shadowSize, FLinearColor shadowColor, bool center, bool outline);
    typedef FRotator (*tGetCameraRotation)(void* cameraManager);
    typedef FVector (*tGetCameraLocation)(void* cameraManager);
    typedef float (*tGetDistanceTo)(void* from, void* to);

    // ── Resolved function pointers ──
    extern tProjectWorldLocationToScreen ProjectWorldLocationToScreen;
    extern tLineOfSightTo LineOfSightTo;
    extern tGetBonePosWithRotation GetBonePos;
    extern tK2_DrawLine DrawLine;
    extern tK2_DrawText DrawText;
    extern tGetCameraRotation GetCameraRotation;
    extern tGetCameraLocation GetCameraLocation;
    extern tGetDistanceTo GetDistanceTo;

    // ── Initialization ──
    bool init(uintptr_t ue4Base);

    // ── Game data accessors ──
    void* getGWorld();
    void* getLocalPlayer();
    void* getLocalPawn();
    void* getPlayerController();
    void* getCameraManager();
    void* getCanvas();
    void* getLevel();
    void** getActors(u32& count);

    // ── Actor info ──
    bool isPlayer(void* actor);
    bool isAlive(void* actor);
    bool isTeammate(void* actor);
    bool isVisible(void* from, void* to);
    FVector getBonePosition(void* mesh, int boneIndex);
    float getHealth(void* actor);
    int getTeamNumber(void* actor);
    FVector getPosition(void* actor);
    const wchar_t* getPlayerName(void* playerState);

} // namespace sdk
