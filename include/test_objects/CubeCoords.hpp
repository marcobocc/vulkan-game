#pragma once
#include <array>
#include <cassert>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>

struct CubeCoords {
    CubeCoords() = default;
    explicit CubeCoords(glm::ivec3 c) : vec_(c) { assert(glm::compAdd(c) == 0); }
    CubeCoords(int x, int y, int z) : vec_(x, y, z) { assert(x + y + z == 0); }

    CubeCoords operator+(const CubeCoords& other) const { return CubeCoords(vec_ + other.vec_); }
    CubeCoords operator-(const CubeCoords& other) const { return CubeCoords(vec_ - other.vec_); }
    CubeCoords operator-() const { return CubeCoords(-vec_); }
    CubeCoords operator*(const int& other) const { return CubeCoords(vec_ * other); }
    CubeCoords operator/(const int& other) const { return CubeCoords(vec_ / other); }
    bool operator==(const CubeCoords& other) const { return vec_ == other.vec_; }
    bool operator!=(const CubeCoords& other) const { return vec_ != other.vec_; }
    std::size_t hash() const { return std::hash<glm::ivec3>()(vec_); }
    glm::ivec3 vec() const { return vec_; }

private:
    glm::ivec3 vec_;
    friend int cubeDistance(const CubeCoords& a, const CubeCoords& b);
};

// --------------------------------
// Make CubeCoords hashable for maps
// --------------------------------
template<>
struct std::hash<CubeCoords> {
    std::size_t operator()(const CubeCoords& c) const noexcept { return c.hash(); }
};

// Pointy Orientation
inline CubeCoords getRightDirection() { return {1, 0, -1}; }
inline CubeCoords getTopRightDirection() { return {1, -1, 0}; }
inline CubeCoords getTopLeftDirection() { return {0, -1, 1}; }

inline CubeCoords getBottomLeftDirection() { return -getTopRightDirection(); }
inline CubeCoords getLeftDirection() { return -getRightDirection(); }
inline CubeCoords getBottomRightDirection() { return -getTopLeftDirection(); }


inline static const std::array<CubeCoords, 6> CUBE_DIRECTIONS = {{getRightDirection(),
                                                                  getTopRightDirection(),
                                                                  getTopLeftDirection(),
                                                                  getLeftDirection(),
                                                                  getBottomLeftDirection(),
                                                                  getBottomRightDirection()}};

// --------------------------------
// Distance
// --------------------------------
inline int cubeDistance(const CubeCoords& a, const CubeCoords& b) { return glm::compMax(glm::abs(a.vec_ - b.vec_)); }

// --------------------------------
// Neighbor retrieval
// --------------------------------
inline std::array<CubeCoords, CUBE_DIRECTIONS.size()> neighbors(const CubeCoords& c) {
    std::array<CubeCoords, CUBE_DIRECTIONS.size()> result{};
    for (size_t i = 0; i < CUBE_DIRECTIONS.size(); ++i)
        result.at(i) = c + CUBE_DIRECTIONS.at(i);
    return result;
}

// --------------------------------
// Ring of radius r
// --------------------------------
inline std::vector<CubeCoords> cubeRing(const CubeCoords& center, int radius) {
    std::vector<CubeCoords> results;
    if (radius == 0) {
        results.push_back(center);
        return results;
    }

    CubeCoords hex = center + CUBE_DIRECTIONS.at(4) * radius;
    for (auto side: CUBE_DIRECTIONS) {
        for (int step = 0; step < radius; ++step) {
            results.push_back(hex);
            hex = hex + side;
        }
    }
    return results;
}

// --------------------------------
// Get all hexes within distance r
// --------------------------------
inline std::vector<CubeCoords> cubeRange(const CubeCoords& center, int radius) {
    std::vector<CubeCoords> results;
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dy = std::max(-radius, -dx - radius); dy <= std::min(radius, -dx + radius); ++dy) {
            int dz = -dx - dy;
            results.push_back(center + CubeCoords(dx, dy, dz));
        }
    }
    return results;
}
