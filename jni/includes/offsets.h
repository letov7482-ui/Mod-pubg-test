#pragma once

// ═══════════════════════════════════════════════
// PUBG Mobile GL 4.6.0 — 64 BIT
// ═══════════════════════════════════════════════

// ── libUE4.so offsets ──
#define OFF_Message_Box              0x8A0C530UL
#define OFF_ShootBulletInner         0x6FF841CUL
#define OFF_FakeDamage_Fix           0xD1DE578UL
#define OFF_CalcShootRot             0x703F6F4UL
#define OFF_ShootGrenadeBullet       0x6F9D99CUL
#define OFF_UpdateVolley             0x704C438UL
#define OFF_LaunchBp                 0x76D3594UL
#define OFF_LobbySkinPlayer          0xA9AFA4CUL
#define OFF_LobbyWeaponPlayer        0x669F9C0UL
#define OFF_ProjectWorldToScreen     0xABD64A0UL
#define OFF_KillMessage              0x67759B0UL
#define OFF_K2_DrawLine              0xAE91B38UL
#define OFF_K2_DrawText              0xAE91DC8UL
#define OFF_K2_DrawTexture           0xAE91BBCUL
#define OFF_LineOfSightTo            0xA78F734UL
#define OFF_GetMuzzleTransform       0x70116DCUL
#define OFF_GetBonePos               0x6746CCCUL
#define OFF_GetBoneName              0xA75196CUL
#define OFF_GetDistanceTo            0xA4E5F6CUL
#define OFF_GetCameraRotation        0x6278EECUL
#define OFF_GetCameraLocation        0x6351BE8UL
#define OFF_GetAndroidSoVersion      0x7D35F68UL
#define OFF_Termination_Fix_1        0x60FF69CUL
#define OFF_Termination_Fix_2        0x6F68470UL
#define OFF_6Hr_TimeFix              0x7D97F60UL
#define OFF_Ping_Fix                 0xC94CFB8UL
#define OFF_Recoil_Small             0x62DC9C4UL
#define OFF_SmallCross               0x62DBDE0UL
#define OFF_Aimbot                   0x8B0913CUL
#define OFF_InstantHit               0xA7180FCUL
#define OFF_Unlock_120fps            0x6AF0378UL
#define OFF_Unlock_Hdr               0x6AF009CUL
#define OFF_Ipad_View                0xA67300CUL
#define OFF_No_Grass_Tree            0x8B01808UL
#define OFF_Flash_Speed              0x602C9D4UL
#define OFF_Flash_Speed2             0x602C898UL
#define OFF_Fix_Stuck                0xA675E24UL
#define OFF_Car_Fly                  0xBB237F0UL

// ── libanogs.so offsets (anticheat bypass) ──
#define ANOGS_1   0x46AE30UL
#define ANOGS_2   0x46AE60UL
#define ANOGS_3   0x46AEDCUL
#define ANOGS_4   0x46AEF4UL
#define ANOGS_5   0x46AF30UL

// ── UE4 internal offsets (GWorld, GNames, etc.) ──
// These are relative to libUE4.so base
#define OFF_GWorld                  0xE7B8D28UL
#define OFF_GNames                  0xE4E14E8UL
#define OFF_GUObjectArray           0xDD0C070UL
#define OFF_GEngine                 0xE7A0EF0UL

// ── Actor offsets (from GWorld traversal) ──
#define OFF_ObjectFlags             0x000C
#define OFF_ObjectIndex             0x000A
#define OFF_ObjectClass             0x0010
#define OFF_ObjectName              0x0018
#define OFF_ObjectOuter             0x0020

#define OFF_Actor_RootComponent     0x0388
#define OFF_Actor_Owner             0x00C0
#define OFF_Actor_Mesh              0x0538
#define OFF_Actor_Players           0x01D8
#define OFF_Actor_LocalPlayers      0x0098
#define OFF_Actor_PlayerState       0x03A0
#define OFF_Actor_Position          0x02B4
#define OFF_Actor_Health            0x0954
#define OFF_Actor_TeamNum           0x06E8
#define OFF_Actor_bDead             0x0A39
#define OFF_Actor_MeshComponent     0x0538
#define OFF_Actor_bIsAI             0x07A8

#define OFF_RootComp_Location       0x02D4
#define OFF_RootComp_Rotation       0x02C4

#define OFF_PlayerController        0x0030
#define OFF_PlayerCameraManager     0x0528
#define OFF_AcknowledgedPawn        0x0550

#define OFF_CameraCache_POV         0x1210
#define OFF_CameraCache_Location    0x1220
#define OFF_CameraCache_Rotation    0x1214
#define OFF_CameraCache_FOV         0x122C

// ── Weapon offsets ──
#define OFF_Weapon_EquippedWeapon   0x0B78
#define OFF_Weapon_AmmoCount        0x0AC8
#define OFF_Weapon_CurFireMode      0x0B18

// ── Bone positions ──
#define BONE_HEAD        0x6
#define BONE_NECK        0x5
#define BONE_CHEST       0x4
#define BONE_PELVIS      0x0
#define BONE_LSHOULDER   0xA
#define BONE_RSHOULDER   0xB
#define BONE_LELBOW      0xC
#define BONE_RELBOW      0xD
#define BONE_LHAND       0xE
#define BONE_RHAND       0xF
#define BONE_LTHIGH      0x14
#define BONE_RTHIGH      0x15
#define BONE_LFOOT       0x18
#define BONE_RFOOT       0x19
