#pragma once
#include <unordered_map>
#include "test_objects/CubeCoords.hpp"

struct Cell {
    std::string name;
    struct {
        uint8_t r, g, b;
    } color{255, 255, 255};
};

struct HexMap {
    std::unordered_map<CubeCoords, Cell> tiles;
};

inline HexMap createDemoHexMap() {
    HexMap map;
    for (int q = -2; q <= 2; ++q) {
        for (int r = -2; r <= 2; ++r) {
            int s = -q - r;
            if (std::abs(s) > 2) continue;
            Cell cell;
            cell.name = "hex";
            cell.color = {(uint8_t) (128 + 32 * q), (uint8_t) (128 + 32 * r), (uint8_t) (128 + 32 * s)};
            map.tiles[CubeCoords(q, r, s)] = cell;
        }
    }
    return map;
}
