#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/triangle_test/TriangleMaterial.hpp"

struct TriangleObject {
    TriangleObject(VkDevice device, VkPhysicalDevice physicalDevice, VkRenderPass renderPass) :
        device_(device),
        physicalDevice_(physicalDevice),
        vertexBuffer_(std::make_unique<VulkanBuffer>(device_,
                                                     physicalDevice_,
                                                     sizeof(TRIANGLE_VERTICES),
                                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                     TRIANGLE_VERTICES)),
        material_(std::make_unique<TriangleMaterial>(device_, renderPass)) {}

    VkBuffer getVkBuffer() const { return vertexBuffer_->getVkBuffer(); }

    TriangleMaterial* getMaterial() const { return material_.get(); }

private:
    static constexpr float TRIANGLE_VERTICES[9] = {0.0f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f};

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    std::unique_ptr<VulkanBuffer> vertexBuffer_;
    std::unique_ptr<TriangleMaterial> material_;
};
