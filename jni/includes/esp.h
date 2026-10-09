#pragma once

#include "types.h"
#include "sdk.h"

namespace esp {

    struct Config {
        bool enabled;
        bool box;
        bool skeleton;
        bool healthBar;
        bool name;
        bool distance;
        bool line;
        bool weapon;
        bool deadBody;
        bool vehicle;
        bool lootBox;
        bool grenade;
        float maxDistance; // meters
    };

    extern Config config;

    void init();
    void render(void* canvas);
    void drawBox(void* canvas, FVector2D top, FVector2D bottom, FLinearColor color, float thickness);
    void drawSkeleton(void* canvas, void* mesh, FVector2D* screenBones, FLinearColor color);
    void drawHealthBar(void* canvas, FVector2D top, FVector2D bottom, float health, float maxHealth);
    void drawName(void* canvas, FVector2D pos, const wchar_t* name, FLinearColor color);
    void drawDistance(void* canvas, FVector2D pos, float dist);
    void drawLineToPlayer(void* canvas, FVector2D screenBottom, FVector2D playerPos, FLinearColor color);
    void drawSnapline(void* canvas, FVector2D screenPos, FLinearColor color);

} // namespace esp
