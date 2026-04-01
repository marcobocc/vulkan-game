#include "vulkan/tilemap_test/VulkanHexMapObject.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include "vulkan/tilemap_test/VulkanHexMapMaterial.hpp"

namespace {
    constexpr float HEX_SIZE = 0.1f;
    constexpr int HEX_VERTICES = 6;

    // Generates the 6 corners of a hex in 2D (z=0)
    std::vector<glm::vec2> hexCorners(float size) {
        std::vector<glm::vec2> corners;
        for (int i = 0; i < HEX_VERTICES; ++i) {
            float angle = glm::radians(60.0f * i - 30.0f);
            corners.emplace_back(size * std::cos(angle), size * std::sin(angle));
        }
        return corners;
    }

    // Converts cube coordinates to 2D world position (pointy-topped)
    glm::vec2 cubeToWorld(const CubeCoords& c, float size) {
        float x = size * (std::sqrt(3.0f) * c.vec().x + std::sqrt(3.0f) / 2 * c.vec().z);
        float y = size * (3.0f / 2 * c.vec().z);
        return {x, y};
    }
} // namespace

VulkanHexMapObject::VulkanHexMapObject(VkDevice device,
                                       VkPhysicalDevice physicalDevice,
                                       VkRenderPass renderPass,
                                       const HexMapComponent& hexMap) :
    device_(device),
    physicalDevice_(physicalDevice),
    material_(std::make_unique<VulkanHexMapMaterial>(device, renderPass)) {
    generateVertices(hexMap);
    vertexBuffer_ =
            std::make_unique<VulkanBuffer>(device_,
                                           physicalDevice_,
                                           sizeof(float) * vertices_.size(),
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           vertices_.data());
    vertexCount_ = vertices_.size() / 5; // 2D pos (x,y) + color (r,g,b)
}

VulkanHexMapObject::~VulkanHexMapObject() = default;

VkBuffer VulkanHexMapObject::getVkBuffer() const { return vertexBuffer_->getVkBuffer(); }
size_t VulkanHexMapObject::getVertexCount() const { return vertexCount_; }
VulkanHexMapMaterial* VulkanHexMapObject::getMaterial() const { return material_.get(); }

void VulkanHexMapObject::updateVertices(const HexMapComponent& hexMap) {
    generateVertices(hexMap);
    // TODO: update buffer with new data (map/unmap or recreate)
}

void VulkanHexMapObject::generateVertices(const HexMapComponent& hexMap) {
    vertices_.clear();
    auto corners = hexCorners(HEX_SIZE);
    for (const auto& [cube, cell]: hexMap.tiles) {
        glm::vec2 center = cubeToWorld(cube, HEX_SIZE);
        for (int i = 0; i < HEX_VERTICES; ++i) {
            glm::vec2 p0 = center;
            glm::vec2 p1 = center + corners[i];
            glm::vec2 p2 = center + corners[(i + 1) % HEX_VERTICES];
            // Each triangle: 3 vertices, each with pos (x,y) and color (r,g,b)
            for (const auto& p: {p0, p1, p2}) {
                vertices_.push_back(p.x);
                vertices_.push_back(p.y);
                vertices_.push_back(cell.color.r / 255.0f);
                vertices_.push_back(cell.color.g / 255.0f);
                vertices_.push_back(cell.color.b / 255.0f);
            }
        }
    }
}
