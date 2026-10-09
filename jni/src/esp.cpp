#include "../includes/esp.h"
#include "../includes/sdk.h"
#include "../includes/offsets.h"
#include "../includes/memory.h"
#include <android/log.h>

#define TAG "PMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

namespace esp {

    Config config = {
        .enabled = true,
        .box = true,
        .skeleton = true,
        .healthBar = true,
        .name = true,
        .distance = true,
        .line = false,
        .weapon = false,
        .deadBody = false,
        .vehicle = true,
        .lootBox = true,
        .grenade = true,
        .maxDistance = 300.0f
    };

    // Bone connection pairs for skeleton
    static const int bonePairs[][2] = {
        // Head to Chest
        {BONE_HEAD, BONE_NECK},
        {BONE_NECK, BONE_CHEST},
        {BONE_CHEST, BONE_PELVIS},
        // Arms
        {BONE_CHEST, BONE_LSHOULDER},
        {BONE_CHEST, BONE_RSHOULDER},
        {BONE_LSHOULDER, BONE_LELBOW},
        {BONE_RSHOULDER, BONE_RELBOW},
        {BONE_LELBOW, BONE_LHAND},
        {BONE_RELBOW, BONE_RHAND},
        // Legs
        {BONE_PELVIS, BONE_LTHIGH},
        {BONE_PELVIS, BONE_RTHIGH},
        {BONE_LTHIGH, BONE_LFOOT},
        {BONE_RTHIGH, BONE_RFOOT},
    };
    static const int bonePairCount = sizeof(bonePairs) / sizeof(bonePairs[0]);

    void init() {
        LOGI("[ESP] Initialized");
    }

    void drawBox(void* canvas, FVector2D top, FVector2D bottom, FLinearColor color, float thickness) {
        if (!canvas) return;

        float width = (bottom.Y - top.Y) / 4.0f; // Box width based on height
        float x = top.X - width / 2.0f;
        float y = top.Y;
        float w = width;
        float h = bottom.Y - top.Y;

        // 4 lines to draw a box
        sdk::DrawLine(canvas, {x, y}, {x + w, y}, thickness, color);          // top
        sdk::DrawLine(canvas, {x, y + h}, {x + w, y + h}, thickness, color);  // bottom
        sdk::DrawLine(canvas, {x, y}, {x, y + h}, thickness, color);          // left
        sdk::DrawLine(canvas, {x + w, y}, {x + w, y + h}, thickness, color); // right
    }

    void drawSkeleton(void* canvas, void* mesh, FVector2D* screenBones, FLinearColor color) {
        if (!canvas || !mesh) return;

        for (int i = 0; i < bonePairCount; i++) {
            int a = bonePairs[i][0];
            int b = bonePairs[i][1];
            if (screenBones[a].X == 0 && screenBones[a].Y == 0) continue;
            if (screenBones[b].X == 0 && screenBones[b].Y == 0) continue;

            sdk::DrawLine(canvas, screenBones[a], screenBones[b], 1.5f, color);
        }
    }

    void drawHealthBar(void* canvas, FVector2D top, FVector2D bottom, float health, float maxHealth) {
        if (!canvas || maxHealth <= 0) return;

        float x = top.X - ((bottom.Y - top.Y) / 4.0f) / 2.0f - 8.0f;
        float y = top.Y;
        float h = bottom.Y - top.Y;
        float barWidth = 4.0f;

        // Background (red - full health bar area)
        FLinearColor bg(0.2f, 0.0f, 0.0f, 0.8f);
        sdk::DrawLine(canvas, {x, y}, {x, y + h}, barWidth + 2.0f, bg);

        // Health (green -> yellow -> red)
        float healthRatio = health / maxHealth;
        if (healthRatio > 1.0f) healthRatio = 1.0f;
        if (healthRatio < 0.0f) healthRatio = 0.0f;

        FLinearColor hpColor;
        if (healthRatio > 0.5f) {
            hpColor = {1.0f - (healthRatio - 0.5f) * 2.0f, 1.0f, 0.0f, 1.0f};
        } else {
            hpColor = {1.0f, healthRatio * 2.0f, 0.0f, 1.0f};
        }

        float healthHeight = h * healthRatio;
        sdk::DrawLine(canvas, {x, y + h - healthHeight}, {x, y + h}, barWidth, hpColor);
    }

    void drawName(void* canvas, FVector2D pos, const wchar_t* name, FLinearColor color) {
        if (!canvas || !name) return;
        // Use K2_DrawText with center alignment
        // We need the font object - we'll use a cached reference
        static void* gameFont = nullptr;
        if (!gameFont) {
            // Try to get font from engine
            // For now, we'll use a fallback
        }
        // Draw text centered above box
        sdk::DrawText(canvas, nullptr, name, {pos.X, pos.Y - 5.0f}, color, 1.0f, {0, 0}, {0,0,0,0}, true, true);
    }

    void drawDistance(void* canvas, FVector2D pos, float dist) {
        if (!canvas) return;

        wchar_t buf[32];
        swprintf(buf, 32, L"[%.0fm]", dist);

        FLinearColor color(0.8f, 0.8f, 0.8f, 1.0f);
        sdk::DrawText(canvas, nullptr, buf, {pos.X, pos.Y + 5.0f}, color, 1.0f, {0, 0}, {0,0,0,0}, true, false);
    }

    void drawLineToPlayer(void* canvas, FVector2D screenBottom, FVector2D playerPos, FLinearColor color) {
        if (!canvas) return;
        // Draw a line from bottom of screen to player
        FVector2D screenCenterBottom = {screenBottom.X / 2.0f, screenBottom.Y};
        sdk::DrawLine(canvas, screenCenterBottom, playerPos, 1.0f, color);
    }

    void drawSnapline(void* canvas, FVector2D screenPos, FLinearColor color) {
        // Simple snapline to bottom center
    }

    void render(void* canvas) {
        if (!config.enabled || !canvas) return;

        void* localPawn = sdk::getLocalPawn();
        if (!localPawn) return;

        void* cameraManager = sdk::getCameraManager();
        if (!cameraManager) return;

        void* playerController = sdk::getPlayerController();
        if (!playerController) return;

        u32 actorCount = 0;
        void** actors = (void**)sdk::getActors(actorCount);
        if (!actors || actorCount == 0) return;

        FVector localPos = sdk::getPosition(localPawn);
        int localTeam = sdk::getTeamNumber(localPawn);

        // Get screen size
        // We'll use a fixed reasonable size for PUBG Mobile
        float screenW = 1080.0f; // Adjust based on device
        float screenH = 2400.0f;
        FVector2D screenBottom(screenW, screenH);

        for (u32 i = 0; i < actorCount; i++) {
            void* actor = actors[i];
            if (!actor || actor == localPawn) continue;

            if (!sdk::isPlayer(actor)) continue;
            if (!sdk::isAlive(actor)) continue;
            if (sdk::isTeammate(actor)) continue;

            FVector actorPos = sdk::getPosition(actor);
            float dist = (actorPos - localPos).Size() / 100.0f; // cm to meters

            if (dist > config.maxDistance) continue;
            if (dist < 1.0f) continue;

            // Get team color
            int team = sdk::getTeamNumber(actor);
            bool visible = sdk::isVisible(localPawn, actor);

            FLinearColor boxColor = visible ?
                FLinearColor::Red() : FLinearColor::Yellow();
            FLinearColor skelColor = visible ?
                FLinearColor::White() : FLinearColor::Yellow();

            // Project head and feet positions
            FVector2D screenTop, screenBottom2;

            void* mesh = *(void**)((uintptr_t)actor + OFF_Actor_Mesh);
            if (!mesh) continue;

            FVector headPos = sdk::getBonePosition(mesh, BONE_HEAD);
            FVector footPos = sdk::getBonePosition(mesh, BONE_LFOOT);

            // Project to screen
            bool bTop = sdk::ProjectWorldLocationToScreen(
                playerController, headPos, screenTop, false);
            bool bBottom = sdk::ProjectWorldLocationToScreen(
                playerController, footPos, screenBottom2, false);

            if (!bTop || !bBottom) continue;
            if (screenTop.Y > screenBottom2.Y) {
                FVector2D temp = screenTop;
                screenTop = screenBottom2;
                screenBottom2 = temp;
            }

            // Draw Box
            if (config.box) {
                drawBox(canvas, screenTop, screenBottom2, boxColor, 1.5f);
            }

            // Draw Skeleton
            if (config.skeleton) {
                FVector2D screenBones[32];
                int boneIndices[] = {BONE_HEAD, BONE_NECK, BONE_CHEST, BONE_PELVIS,
                    BONE_LSHOULDER, BONE_RSHOULDER, BONE_LELBOW, BONE_RELBOW,
                    BONE_LHAND, BONE_RHAND, BONE_LTHIGH, BONE_RTHIGH,
                    BONE_LFOOT, BONE_RFOOT};

                for (int b = 0; b < 14 && b < 32; b++) {
                    FVector bonePos = sdk::getBonePosition(mesh, boneIndices[b]);
                    sdk::ProjectWorldLocationToScreen(playerController, bonePos, screenBones[boneIndices[b]], false);
                }
                drawSkeleton(canvas, mesh, screenBones, skelColor);
            }

            // Draw Health Bar
            if (config.healthBar) {
                float health = sdk::getHealth(actor);
                drawHealthBar(canvas, screenTop, screenBottom2, health, 100.0f);
            }

            // Draw Name
            if (config.name) {
                void* playerState = *(void**)((uintptr_t)actor + OFF_Actor_PlayerState);
                if (playerState) {
                    const wchar_t* name = sdk::getPlayerName(playerState);
                    if (name) {
                        drawName(canvas, screenTop, name, FLinearColor::White());
                    }
                }
            }

            // Draw Distance
            if (config.distance) {
                drawDistance(canvas, screenBottom2, dist);
            }

            // Draw Snapline
            if (config.line) {
                drawLineToPlayer(canvas, screenBottom, screenBottom2,
                    visible ? FLinearColor::Red() : FLinearColor::Green());
            }
        }
    }

} // namespace esp
