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

    struct UActorComponent : public UObject {
        u8 pad[0x08];            // 0x0028
        void* OwnerPrivate;      // 0x0030
    };

    struct USceneComponent : public UActorComponent {
        u8 pad[0x280];           // to 0x02B4
        FVector RelativeLocation; // 0x02B4 (offset from base)
        // ... more fields
    };

    struct UPrimitiveComponent : public USceneComponent {
        // contains mesh data
    };

    struct USkeletalMeshComponent : public UPrimitiveComponent {
        // bone array access
    };

    struct ULocalPlayer : public UObject {
        void* Controller;        // offset varies
    };

    struct APlayerController : public UObject {
        // Contains CameraManager, Pawn, etc.
    };

    struct APlayerCameraManager : public UObject {
        // CameraCacheEntry
    };

    struct APawn : public UObject {
        // Root component, Health, TeamNum
    };

    struct ACharacter : public APawn {
        // Mesh component
    };

    struct UCanvas : public UObject {
        // K2_DrawText, K2_DrawLine functions
    };

    // ── Helper functions (resolved from offsets) ──
    // These are function pointers we'll resolve at runtime

    typedef bool (*tProjectWorldLocationToScreen)(void* playerController, FVector& worldPos, FVector2D& screenPos, bool relative);
    typedef bool (*tLineOfSightTo)(void* actor, void* other, FVector viewPoint);
    typedef FVector (*tGetBonePosWithRotation)(void* mesh, int boneIndex);
    typedef void (*tK2_DrawLine)(void* canvas, FVector2D screenPosA, FVector2D screenPosB, float thickness, FLinearColor color);
    typedef void (*tK2_DrawText)(void* canvas, void* font, const wchar_t* text, FVector2D screenPos, FLinearColor color, float kerning, FVector2D shadowSize, FLinearColor shadowColor, bool center, bool outline);
    typedef FRotator (*tGetCameraRotation)(void* cameraManager);
    typedef FVector (*tGetCameraLocation)(void* cameraManager);
    typedef float (*tGetDistanceTo)(void* from, void* to);

    // FLinearColor
    struct FLinearColor {
        float R, G, B, A;
        FLinearColor() : R(0), G(0), B(0), A(1) {}
        FLinearColor(float r, float g, float b, float a) : R(r), G(g), B(b), A(a) {}

        static FLinearColor Red()    { return {1.0f, 0.0f, 0.0f, 1.0f}; }
        static FLinearColor Green()  { return {0.0f, 1.0f, 0.0f, 1.0f}; }
        static FLinearColor Blue()   { return {0.0f, 0.0f, 1.0f, 1.0f}; }
        static FLinearColor Yellow() { return {1.0f, 1.0f, 0.0f, 1.0f}; }
        static FLinearColor White()  { return {1.0f, 1.0f, 1.0f, 1.0f}; }
        static FLinearColor Cyan()   { return {0.0f, 1.0f, 1.0f, 1.0f}; }
        static FLinearColor Purple() { return {0.5f, 0.0f, 0.5f, 1.0f}; }
        static FLinearColor Orange() { return {1.0f, 0.5f, 0.0f, 1.0f}; }
        static FLinearColor Pink()   { return {1.0f, 0.4f, 0.7f, 1.0f}; }
    };

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
    void* getActors(u32& count);

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
