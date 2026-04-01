#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vulkan/VulkanCommandManager.hpp"
#include "vulkan/VulkanDebugMessenger.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanPipelinesManager.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"
#include "vulkan/VulkanVertexBuffersManager.hpp"
#include "vulkan/test_objects/VulkanHexMap.hpp"
#include "vulkan/test_objects/VulkanTriangle.hpp"

class VulkanGraphicsBackend {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    VulkanGraphicsBackend(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend& operator=(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend&&) = delete;
    VulkanGraphicsBackend& operator=(VulkanGraphicsBackend&&) = delete;

    ~VulkanGraphicsBackend();
    explicit VulkanGraphicsBackend(GLFWwindow* window);
    void renderFrame();

private:
    size_t currentFrame_ = 0;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores_{};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores_{};
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences_{};
    GLFWwindow* window_ = nullptr; // TODO: Create window wrapper object
    VulkanInstance instance_;
    VulkanDebugMessenger debugMessenger_;
    VulkanDevice device_;
    VulkanCommandManager commandManager_;
    VulkanSwapchainManager swapchainManager_;
    VulkanPipelinesManager pipelinesManager_;
    VulkanVertexBuffersManager vertexBuffersManager_;

    // TODO: Move out of this class
    VulkanTriangle triangleObject_;
    VulkanHexMap hexMapObject_;
};
