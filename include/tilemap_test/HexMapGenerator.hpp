#pragma once
#include "ecs/components/HexMapComponent.hpp"

inline HexMapComponent createDemoHexMap() {
    HexMapComponent map;
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
