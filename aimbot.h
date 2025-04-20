#pragma once

#include <Windows.h>
#include <cstdint>

struct Vector3 {
    float x, y, z;

    float Distance(const Vector3& other) const;
    Vector3 operator-(const Vector3& rhs) const;
};

extern HANDLE hProc;
extern uintptr_t baseAddress;
extern bool aimbotEnabled;

bool ReadVec3(uintptr_t addr, Vector3& vec);
bool ReadHead(uintptr_t addr, Vector3& vec);
Vector3 CalcAngle(const Vector3& from, const Vector3& to);
void AimAtTarget(uintptr_t localPlayer, const Vector3& currentAngle, const Vector3& targetAngle, float smoothPercent);
bool WorldToScreen(const Vector3& pos, Vector3& screen, float matrix[16], int width, int height);
void Tick();