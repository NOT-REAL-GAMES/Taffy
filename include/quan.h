#pragma once

#include <glm/glm.hpp>


#pragma pack(push, 1)

struct Vec3Q {
    uint64_t x, y, z;

    Vec3Q() : x(0), y(0), z(0) {}
    Vec3Q(int64_t x, int64_t y, int64_t z, bool raw = false) : x(x+9223372036854775808ULL), y(y+9223372036854775808ULL), z(z+9223372036854775808ULL) {}
    Vec3Q(glm::vec3 v) : x(v.x*128000+9223372036854775808ULL),y(v.y*128000+9223372036854775808ULL),z(v.z*128000+9223372036854775808ULL) {}

    // Conversion from floating point to quantized
    static Vec3Q fromFloat(const glm::vec3& v) {
        // Convert from meters to 1/128 mm
        // 1 unit = 1/128 millimeter = 0.0078125 mm
        constexpr double scale = 128000.0; // 128 units per mm * 1000 mm per meter
        return {
            static_cast<int64_t>(v.x * scale),
            static_cast<int64_t>(v.y * scale),
            static_cast<int64_t>(v.z * scale)
        };
    }

    // Conversion from quantized to floating point
    glm::vec3 toFloat() const {
        constexpr double invScale = 1.0 / 128000.0;
        constexpr uint64_t offset = 9223372036854775808ULL;
        return glm::vec3(
            static_cast<float>(static_cast<int64_t>(x - offset) * invScale),
            static_cast<float>(static_cast<int64_t>(y - offset) * invScale),
            static_cast<float>(static_cast<int64_t>(z - offset) * invScale)
        );
    }

    // Basic vector operations
    Vec3Q operator+(const Vec3Q& other) const {
        return Vec3Q(x + other.x, y + other.y, z + other.z);
    }

    Vec3Q operator+=(const Vec3Q& other) const {
        return *this + other;
    }

    Vec3Q operator-(const Vec3Q& other) const {
        return Vec3Q(x - other.x, y - other.y, z - other.z);
    }

    Vec3Q operator-=(const Vec3Q& other) const {
            return *this - other;
    }
    
    Vec3Q operator*(int64_t scalar) const {
        return Vec3Q(x * scalar, y * scalar, z * scalar);
    }

    Vec3Q operator/(int64_t scalar) const {
        return Vec3Q(x / scalar, y / scalar, z / scalar);
    }

    // Comparisons (useful for sorting in spatial data structures)
    bool operator==(const Vec3Q& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Vec3Q& other) const {
        return !(*this == other);
    }

    glm::vec3 relativeTo(const Vec3Q& origin) const {
        constexpr double invScale = 1.0 / 128000.0;
        return glm::vec3(
            static_cast<float>(static_cast<int64_t>(x - origin.x) * invScale),
            static_cast<float>(static_cast<int64_t>(y - origin.y) * invScale),
            static_cast<float>(static_cast<int64_t>(z - origin.z) * invScale)
        );

    }

    Vec3Q lerp(const Vec3Q& other, double t) const {
    constexpr uint64_t offset = 9223372036854775808ULL;
    int64_t ax = static_cast<int64_t>(x - offset);
    int64_t ay = static_cast<int64_t>(y - offset);
    int64_t az = static_cast<int64_t>(z - offset);
    int64_t bx = static_cast<int64_t>(other.x - offset);
    int64_t by = static_cast<int64_t>(other.y - offset);
    int64_t bz = static_cast<int64_t>(other.z - offset);

    return Vec3Q(
        static_cast<int64_t>(ax + (bx - ax) * t),
        static_cast<int64_t>(ay + (by - ay) * t),
        static_cast<int64_t>(az + (bz - az) * t)
    );


}

};

#pragma pack(pop)
