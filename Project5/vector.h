//Tento header subor som nepisal ja

#pragma once
#ifndef _VECTOR_ESP_
#define _VECTOR_ESP_
#include <cmath>
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <fstream>
#include <string>
#include <limits>
#include <d3d9.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <wtypes.h>
#include <cstddef>

constexpr float PI = 3.14159265358979323846f;

struct view_matrix_t {
    float* operator[](int index) {
        return matrix[index];
    }
    float matrix[4][4];
};

struct Vector3 {
    constexpr Vector3(const float x = 0.f, const float y = 0.f, const float z = 0.f) noexcept
        : x(x), y(y), z(z) {
    }

    constexpr const Vector3& operator-(const Vector3& other) const noexcept {
        return Vector3{ x - other.x, y - other.y, z - other.z };
    }

    constexpr const Vector3& operator+(const Vector3& other) const noexcept {
        return Vector3{ x + other.x, y + other.y, z + other.z };
    }

    constexpr const Vector3& operator/(const float factor) const noexcept {
        return Vector3{ x / factor, y / factor, z / factor };
    }

    constexpr const Vector3& operator*(const float factor) const noexcept {
        return Vector3{ x * factor, y * factor, z * factor };
    }

    constexpr const bool operator>(const Vector3& other) const noexcept {
        return x > other.x && y > other.y && z > other.z;
    }

    constexpr const bool operator>=(const Vector3& other) const noexcept {
        return x >= other.x && y >= other.y && z >= other.z;
    }

    constexpr const bool operator<(const Vector3& other) const noexcept {
        return x < other.x && y < other.y && z < other.z;
    }

    constexpr const bool operator<=(const Vector3& other) const noexcept {
        return x <= other.x && y <= other.y && z <= other.z;
    }

    constexpr const Vector3& ToAngle() const noexcept {
        return Vector3{ std::atan2(-z, std::hypot(x, y)) * (180.0f / PI),
                       std::atan2(y, x) * (180.0f / PI), 0.0f };
    }

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    float length2d() const {
        return std::sqrt(x * x + y * y);
    }

    constexpr const bool IsZero() const noexcept {
        return x == 0.f && y == 0.f && z == 0.f;
    }

    float calculate_distance(const Vector3& point) const {
        float dx = point.x - x;
        float dy = point.y - y;
        float dz = point.z - z;

        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }


    Vector3 WTS(view_matrix_t matrix) const {
        float _x = matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z + matrix[0][3];
        float _y = matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z + matrix[1][3];

        float w = matrix[3][0] * x + matrix[3][1] * y + matrix[3][2] * z + matrix[3][3];

        float inv_w = 1.f / w;
        _x *= inv_w;
        _y *= inv_w;

        float x = GetSystemMetrics(SM_CXSCREEN) / 2;
        float y = GetSystemMetrics(SM_CYSCREEN) / 2;

        x += 0.5f * _x * GetSystemMetrics(SM_CXSCREEN) + 0.5f;
        y -= 0.5f * _y * GetSystemMetrics(SM_CYSCREEN) + 0.5f;

        return { x, y, w };
    }

    float x, y, z;
};

struct vec4 {
    float w, x, y, z;
};

struct vec3 {
    float x, y, z;
    vec3 operator+(vec3 other) {
        return { this->x + other.x, this->y + other.y, this->z + other.z };
    }
    vec3 operator-(vec3 other) {
        return { this->x - other.x, this->y - other.y, this->z - other.z };
    }

    vec3 RelativeAngle() {
        return { std::atan2(-z, std::hypot(x, y)) * (180.0f / PI),
                std::atan2(y, x) * (180.0f / PI), 0.0f };
    }
};

struct vec2 {
    float x, y;
};

#endif
