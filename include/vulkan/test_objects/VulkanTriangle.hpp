#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/VulkanPipelinesManager.hpp"

struct VulkanTriangle {
    VulkanTriangle(VkDevice device, VkPhysicalDevice physicalDevice, VulkanPipelinesManager* pipelinesManager) :
        pipelinesManager_(pipelinesManager),
        device_(device),
        physicalDevice_(physicalDevice),
        vertexBuffer_(std::make_unique<VulkanBuffer>(device_,
                                                     physicalDevice_,
                                                     sizeof(TRIANGLE_VERTICES),
                                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                     TRIANGLE_VERTICES)) {}

    void render(VkCommandBuffer cmd) const {
        VkPipelineVertexInputStateCreateInfo triangleVertexInputInfo{};
        triangleVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        auto* trianglePipelineObj = pipelinesManager_->createOrGetPipeline(
                "triangle", triangleVertexInputInfo, "shaders/triangle.vert.spv", "shaders/triangle.frag.spv");
        VkPipeline trianglePipeline = trianglePipelineObj->getVkPipeline();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
        std::array<VkDeviceSize, 1> offsets = {0};
        VkBuffer triangleVertexBuffer = vertexBuffer_->getVkBuffer();
        vkCmdBindVertexBuffers(cmd, 0, 1, &triangleVertexBuffer, offsets.data());
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }

private:
    static constexpr float TRIANGLE_VERTICES[9] = {0.0f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f};

    VulkanPipelinesManager* pipelinesManager_;
    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    std::unique_ptr<VulkanBuffer> vertexBuffer_;
};
