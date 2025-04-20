#include <Windows.h>
#include <cstdint>
#include <cmath>
#include <limits>
#include "constants.h"
#include <algorithm>

HANDLE hProc;
uintptr_t baseAddress;

struct Vector3 {
    float x, y, z;

    float Distance(const Vector3& other) const {
        return sqrtf((x - other.x) * (x - other.x) +
                     (y - other.y) * (y - other.y) +
                     (z - other.z) * (z - other.z));
    }

    Vector3 operator-(const Vector3& rhs) const {
        return { x - rhs.x, y - rhs.y, z - rhs.z };
    }
};

bool ReadVec3(uintptr_t addr, Vector3& vec) {
    return ReadProcessMemory(hProc, (LPCVOID)(addr + Offsets::PositionX), &vec.x, sizeof(float), nullptr) &&
           ReadProcessMemory(hProc, (LPCVOID)(addr + Offsets::PositionZ), &vec.y, sizeof(float), nullptr) &&
           ReadProcessMemory(hProc, (LPCVOID)(addr + Offsets::PositionY), &vec.z, sizeof(float), nullptr);
}

bool ReadHead(uintptr_t addr, Vector3& vec) {
    return ReadProcessMemory(hProc, (LPCVOID)(addr + Offsets::HeadX), &vec.x, sizeof(float), nullptr) &&
           ReadProcessMemory(hProc, (LPCVOID)(addr + Offsets::HeadZ), &vec.y, sizeof(float), nullptr) &&
           ReadProcessMemory(hProc, (LPCVOID)(addr + Offsets::HeadY), &vec.z, sizeof(float), nullptr);
}

Vector3 CalcAngle(const Vector3& from, const Vector3& to) {
    Vector3 delta = from - to;
    Vector3 angles;
    angles.x = atanf(delta.x / delta.y) * -57.2957795f;
    angles.y = atanf(delta.z / sqrtf(delta.x * delta.x + delta.y * delta.y)) * -57.2957795f;
    angles.z = 0.f;

    if (delta.y < 0.0f) angles.x += 180.0f;
    while (angles.x < 0.f) angles.x += 360.f;
    while (angles.x >= 360.f) angles.x -= 360.f;
    angles.y = std::clamp(angles.y, -90.0f, 90.0f);

    return angles;
}

void AimAtTarget(uintptr_t localPlayer, const Vector3& currentAngle, const Vector3& targetAngle, float smoothPercent) {
    float smooth = std::clamp(smoothPercent, 0.0f, 100.0f) / 100.0f;
    Vector3 result;
    result.x = currentAngle.x + (targetAngle.x - currentAngle.x) * smooth;
    result.y = currentAngle.y + (targetAngle.y - currentAngle.y) * smooth;

    WriteProcessMemory(hProc, (LPVOID)(localPlayer + 0x34), &result.x, sizeof(float), nullptr);
    WriteProcessMemory(hProc, (LPVOID)(localPlayer + 0x38), &result.y, sizeof(float), nullptr);
}

bool WorldToScreen(const Vector3& pos, Vector3& screen, float matrix[16], int width, int height) {
    float w = matrix[3] * pos.x + matrix[7] * pos.y + matrix[11] * pos.z + matrix[15];
    if (w < 0.1f) return false;

    screen.x = matrix[0] * pos.x + matrix[4] * pos.y + matrix[8] * pos.z + matrix[12];
    screen.y = matrix[1] * pos.x + matrix[5] * pos.y + matrix[9] * pos.z + matrix[13];
    screen.z = matrix[2] * pos.x + matrix[6] * pos.y + matrix[10] * pos.z + matrix[14];

    screen.x /= w;
    screen.y /= w;

    screen.x = (width / 2 * screen.x) + (width / 2);
    screen.y = -(height / 2 * screen.y) + (height / 2);

    return true;
}

bool aimbotEnabled = true; // 전역 또는 외부에서 제어할 수 있도록 선언

void Tick() {
    if (!aimbotEnabled) return;
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) return;
    uintptr_t localPlayer;
    ReadProcessMemory(hProc, (LPCVOID)(baseAddress + Offsets::LocalPlayer), &localPlayer, sizeof(localPlayer), nullptr);

    Vector3 localPos;
    if (!ReadVec3(localPlayer, localPos)) return;

    Vector3 currentAngle;
    ReadProcessMemory(hProc, (LPCVOID)(localPlayer + Offsets::CameraX), &currentAngle.x, sizeof(float), nullptr);
    ReadProcessMemory(hProc, (LPCVOID)(localPlayer + Offsets::CameraY), &currentAngle.y, sizeof(float), nullptr);

    uintptr_t entityList;
    ReadProcessMemory(hProc, (LPCVOID)(baseAddress + Offsets::EntityList), &entityList, sizeof(entityList), nullptr);

    int playerCount;
    ReadProcessMemory(hProc, (LPCVOID)(baseAddress + Offsets::PlayerCount), &playerCount, sizeof(int), nullptr);

    float viewMatrix[16];
    ReadProcessMemory(hProc, (LPCVOID)(baseAddress + Offsets::ViewMatrix), &viewMatrix, sizeof(viewMatrix), nullptr);

    int screenWidth = 1920;  // 필요시 GetSystemMetrics 사용 가능
    int screenHeight = 1080;

    uintptr_t bestTarget = 0;
    float bestPriority = FLT_MAX;

    for (int i = 0; i < playerCount; ++i) {
        uintptr_t entity;
        ReadProcessMemory(hProc, (LPCVOID)(entityList + i * 4), &entity, sizeof(entity), nullptr);
        if (entity == 0 || entity == localPlayer) continue;

        int health;
        ReadProcessMemory(hProc, (LPCVOID)(entity + Offsets::Health), &health, sizeof(health), nullptr);
        if (health <= 0) continue;

        Vector3 pos;
        if (!ReadVec3(entity, pos)) continue;

        Vector3 screen;
        if (!WorldToScreen(pos, screen, viewMatrix, screenWidth, screenHeight)) continue;
        if (!WorldToScreen(pos, screen, viewMatrix, screenWidth, screenHeight)) continue;

        float centerX = screenWidth / 2.0f;
        float centerY = screenHeight / 2.0f;
        float dist2D = sqrtf((screen.x - centerX) * (screen.x - centerX) + (screen.y - centerY) * (screen.y - centerY));

        float maxFOVPixels = 150.0f;
        if (dist2D > maxFOVPixels) continue;

                float worldDistance = localPos.Distance(pos);
        float priority = dist2D + worldDistance * 0.5f; // 조준점 + 거리 기반 가중치

        if (priority < bestPriority) {
            bestPriority = priority;
            bestTarget = entity;
        }
    }

    if (bestTarget) {
        Vector3 targetHead;
        if (!ReadHead(bestTarget, targetHead)) return;
        Vector3 angle = CalcAngle(localPos, targetHead);
        float fov = sqrtf(powf(angle.x - currentAngle.x, 2) + powf(angle.y - currentAngle.y, 2));
        float percent = std::clamp(fov / 360.0f * 100.0f, 0.0f, 100.0f);
        AimAtTarget(localPlayer, currentAngle, angle, percent);
    }
}
