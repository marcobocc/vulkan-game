#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "ecs/components/HexMapComponent.hpp"
#include "vulkan/VulkanBuffer.hpp"

class VulkanHexMapObject {
public:
    VulkanHexMapObject(VkDevice device, VkPhysicalDevice physicalDevice, const HexMapComponent& hexMap);
    ~VulkanHexMapObject();

    VkBuffer getVkBuffer() const;
    size_t getVertexCount() const;
    void updateVertices(const HexMapComponent& hexMap);

private:
    void generateVertices(const HexMapComponent& hexMap);

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    std::unique_ptr<VulkanBuffer> vertexBuffer_;
    std::vector<float> vertices_;
    size_t vertexCount_ = 0;
};
