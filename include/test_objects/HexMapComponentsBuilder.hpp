#pragma once
#include <glm/glm.hpp>
#include "HexMap.hpp"
#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"

inline MeshComponent buildHexMapMesh(const HexMap& hexMap) {
    MeshComponent mesh;
    mesh.name = "hexmap";

    constexpr float HEX_SIZE = 0.1f;
    constexpr int HEX_VERTICES = 6;

    auto hexCorners = [&](float size) -> std::vector<glm::vec2> {
        std::vector<glm::vec2> corners;
        for (int i = 0; i < HEX_VERTICES; ++i) {
            float angle = glm::radians(60.0f * i - 30.0f);
            corners.emplace_back(size * std::cos(angle), size * std::sin(angle));
        }
        return corners;
    };

    auto cubeToWorld = [&](const CubeCoords& c, float size) -> glm::vec2 {
        float x = size * (std::sqrt(3.0f) * c.vec().x + std::sqrt(3.0f) / 2.0f * c.vec().z);
        float y = size * (3.0f / 2.0f * c.vec().z);
        return {x, y};
    };

    std::vector<float> vertices;
    auto corners = hexCorners(HEX_SIZE);

    for (const auto& [cube, cell]: hexMap.tiles) {
        glm::vec2 center = cubeToWorld(cube, HEX_SIZE);

        for (int i = 0; i < HEX_VERTICES; ++i) {
            glm::vec2 p0 = center;
            glm::vec2 p1 = center + corners[i];
            glm::vec2 p2 = center + corners[(i + 1) % HEX_VERTICES];

            for (const auto& p: {p0, p1, p2}) {
                vertices.push_back(p.x);
                vertices.push_back(p.y);
                vertices.push_back(cell.color.r / 255.0f);
                vertices.push_back(cell.color.g / 255.0f);
                vertices.push_back(cell.color.b / 255.0f);
            }
        }
    }

    mesh.vertices = std::move(vertices);
    mesh.vertexStride = 5; // vec2 pos + vec3 color
    mesh.attributes = {{"position", 0, 2}, {"color", 2, 3}};

    return mesh;
}

inline MaterialComponent buildHexMapMaterial() {
    MaterialComponent mat;
    mat.name = "hexmap";
    mat.vertexShaderPath = "shaders/hexmap.vert.spv";
    mat.fragmentShaderPath = "shaders/hexmap.frag.spv";
    return mat;
}
