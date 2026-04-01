#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "VulkanHexMapMaterial.hpp"
#include "ecs/components/HexMapComponent.hpp"
#include "vulkan/VulkanBuffer.hpp"

class VulkanHexMapObject {
public:
    VulkanHexMapObject(VkDevice device,
                       VkPhysicalDevice physicalDevice,
                       VkRenderPass renderPass,
                       const HexMapComponent& hexMap);
    ~VulkanHexMapObject();

    VkBuffer getVkBuffer() const;
    size_t getVertexCount() const;
    VulkanHexMapMaterial* getMaterial() const;
    void updateVertices(const HexMapComponent& hexMap);

private:
    void generateVertices(const HexMapComponent& hexMap);

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    std::unique_ptr<VulkanBuffer> vertexBuffer_;
    std::unique_ptr<VulkanHexMapMaterial> material_;
    std::vector<float> vertices_;
    size_t vertexCount_ = 0;
};
