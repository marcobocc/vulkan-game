#pragma once

#include <vulkan/vulkan.h>
#include "vulkan/VulkanPipelinesManager.hpp"
#include "vulkan/VulkanVertexBuffersManager.hpp"

struct VulkanTriangle {
    VulkanTriangle(VulkanPipelinesManager* pipelinesManager, VulkanVertexBuffersManager* vertexBuffersManager) :
        pipelinesManager_(pipelinesManager),
        vertexBuffersManager_(vertexBuffersManager) {}

    void render(VkCommandBuffer cmd) const {
        VkPipelineVertexInputStateCreateInfo triangleVertexInputInfo{};
        triangleVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        auto* trianglePipelineObj = pipelinesManager_->createOrGetPipeline(
                "triangle", triangleVertexInputInfo, "shaders/triangle.vert.spv", "shaders/triangle.frag.spv");
        VkPipeline trianglePipeline = trianglePipelineObj->getVkPipeline();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
        std::array<VkDeviceSize, 1> offsets = {0};

        VkBuffer vertexBuffer =
                vertexBuffersManager_->createOrGetVertexBuffer("triangle", TRIANGLE_VERTICES, sizeof(TRIANGLE_VERTICES))
                        ->getVkBuffer();

        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets.data());
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }

private:
    static constexpr float TRIANGLE_VERTICES[9] = {0.0f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f};

    VulkanPipelinesManager* pipelinesManager_;
    VulkanVertexBuffersManager* vertexBuffersManager_;
};
