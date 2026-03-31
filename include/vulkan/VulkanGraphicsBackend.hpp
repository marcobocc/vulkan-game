#pragma once

#include <log4cxx/logger.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "triangle_test/TriangleMaterial.hpp"
#include "vulkan/VulkanCommandManager.hpp"
#include "vulkan/VulkanDebugMessenger.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"
#include "vulkan/triangle_test/TriangleObject.hpp"

static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanGraphicsBackend");

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

    TriangleObject triangleObject_; // TODO: Move out of this class
};
