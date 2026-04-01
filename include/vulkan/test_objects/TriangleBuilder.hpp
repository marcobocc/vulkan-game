#pragma once

#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"

inline MeshComponent buildTriangleMesh() {
    MeshComponent triangleMesh;
    triangleMesh.name = "triangle";
    triangleMesh.vertices = {
            0.0f,
            -0.5f,
            1,
            0,
            0, // vertex 0: x,y + r,g,b
            0.5f,
            0.5f,
            0,
            1,
            0, // vertex 1
            -0.5f,
            0.5f,
            0,
            0,
            1 // vertex 2
    };
    triangleMesh.vertexStride = 5; // 2 floats pos + 3 floats color
    triangleMesh.attributes = {
            {"position", 0, 2}, // x,y
            {"color", 2, 3} // r,g,b
    };
    return triangleMesh;
}

inline MaterialComponent buildTriangleMaterial() {
    MaterialComponent mat;
    mat.name = "triangle";
    mat.vertexShaderPath = "shaders/triangle.vert.spv";
    mat.fragmentShaderPath = "shaders/triangle.frag.spv";
    return mat;
}
