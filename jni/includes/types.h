#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

struct FVector {
    float X, Y, Z;
    FVector() : X(0), Y(0), Z(0) {}
    FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}
    FVector operator-(const FVector& o) const { return {X-o.X, Y-o.Y, Z-o.Z}; }
    FVector operator+(const FVector& o) const { return {X+o.X, Y+o.Y, Z+o.Z}; }
    FVector operator*(float s) const { return {X*s, Y*s, Z*s}; }
    float Size() const { return sqrtf(X*X + Y*Y + Z*Z); }
    float Size2D() const { return sqrtf(X*X + Y*Y); }
    float Dot(const FVector& o) const { return X*o.X + Y*o.Y + Z*o.Z; }
    FVector Normalize() const {
        float s = Size();
        if (s < 0.001f) return {0,0,0};
        return {X/s, Y/s, Z/s};
    }
};

struct FRotator {
    float Pitch, Yaw, Roll;
    FRotator() : Pitch(0), Yaw(0), Roll(0) {}
    FRotator(float p, float y, float r) : Pitch(p), Yaw(y), Roll(r) {}
};

struct FVector2D {
    float X, Y;
    FVector2D() : X(0), Y(0) {}
    FVector2D(float x, float y) : X(x), Y(y) {}
    float Size() const { return sqrtf(X*X + Y*Y); }
};

struct FMatrix {
    float M[4][4];
};

struct FQuat {
    float X, Y, Z, W;
};

struct FTransform {
    FQuat Rotation;
    FVector Translation;
    float pad;
    FVector Scale3D;
    float pad1;
};

struct FBox {
    FVector Min, Max;
    u8 IsValid;
    u8 pad[7];
};

// FLinearColor — глобально, используется везде
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
