#pragma once
#include <unordered_map>
#include "tilemap_test/CubeCoords.hpp"

struct Cell {
    std::string name;
    struct { uint8_t r, g, b; } color{255,255,255};
};

struct HexMapComponent {
    std::unordered_map<CubeCoords, Cell> tiles;
};
